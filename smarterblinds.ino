#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>
#include <WiFiClient.h>
#include <NTPClient.h>
#include <FS.h>

#define UP_PIN 4
#define DOWN_PIN 5
#define BUTTON_UP_PIN 12
#define BUTTON_DOWN_PIN 14

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
DynamicJsonDocument config(1024);
ESP8266WebServer server(80);

void setup() {
  pinMode(UP_PIN, OUTPUT);
  pinMode(DOWN_PIN, OUTPUT);
  digitalWrite(UP_PIN, HIGH);
  digitalWrite(DOWN_PIN, HIGH);
  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);

  delay(10);
  Serial.begin(115200);

  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  Serial.println("Starting WiFi manager");

  WiFiManager wifiManager;
  wifiManager.autoConnect("SmarterBlinds");

  Serial.println("Connected to WiFi");
  Serial.println("Starting mDNS");

  if (MDNS.begin("smarterblinds")) {
    Serial.println("mDNS responder started");  
  }
  
  timeClient.begin();
  Serial.println("NTP client started");

  File configFile = SPIFFS.open("/config.txt", "r");
  if (configFile) {
    Serial.println("Config file found");
    deserializeJson(config, configFile);
    configFile.close();
    initTimeClientFromConfig();
  }

  server.on("/", handleRoot);
  server.on("/index.html", handleRoot);
  server.on("/blinds.js", handleJS);
  server.on("/blinds.css", handleCSS);
  server.on("/settings", handleSettings);
  server.on("/up", handleUp);
  server.on("/down", handleDown);
  
  server.begin();
  
  Serial.println("Server started");
}

void initTimeClientFromConfig() {
  int offsetHours = config["offsetHours"].as<int>();
  bool dst = config["dst"].as<bool>();
  if (dst) {
    offsetHours = offsetHours + 1;
  }
  int offsetSeconds = offsetHours * 60 * 60;
  timeClient.setTimeOffset(offsetSeconds);
}

int desiredDirection = 0;
int lastDirection = 0;
bool upLatched = false;
bool downLatched = false;
void loop(void) {
  desiredDirection = 0;
  timeClient.update();

  MDNS.update();

  server.handleClient();
  
  int upButtonVal = digitalRead(BUTTON_UP_PIN);
  int downButtonVal = digitalRead(BUTTON_DOWN_PIN);

  if (upButtonVal == LOW || upLatched) {
    desiredDirection = 1;
  }

  if (downButtonVal == LOW || downLatched) {
    desiredDirection = -1;
  }

  if (desiredDirection != lastDirection) {
    Serial.println(String("New direction ") + desiredDirection + " last direction " + lastDirection);
    if (desiredDirection == 0) {
      digitalWrite(UP_PIN, HIGH);
      digitalWrite(DOWN_PIN, HIGH);
    } else if (desiredDirection == 1) {
      digitalWrite(UP_PIN, LOW);
      digitalWrite(DOWN_PIN, HIGH);
    } else if (desiredDirection == -1) {
      digitalWrite(UP_PIN, HIGH);
      digitalWrite(DOWN_PIN, LOW);
    } 
    delay(100); 
  }

  lastDirection = desiredDirection;
  checkSchedule();
}

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
void checkSchedule() {
  const char* currentDoW = daysOfTheWeek[timeClient.getDay()];
  if (config["schedule"][currentDoW] != NULL) {
    //Serial.println(String(currentDoW) + " " + timeClient.getHours() + " " + timeClient.getMinutes() + " " + String(config["schedule"][currentDoW]["localHours"].as<int>()) + " " + String(config["schedule"][currentDoW]["localMinutes"].as<int>()));
    if (timeClient.getHours() == config["schedule"][currentDoW]["localHours"].as<int>()) {
      if (timeClient.getMinutes() == config["schedule"][currentDoW]["localMinutes"].as<int>()) {
        if (timeClient.getSeconds() >= 0 && timeClient.getSeconds() <= 2) {
          Serial.println("Schedule match");
          digitalWrite(UP_PIN, LOW);
          delay(100);
          digitalWrite(UP_PIN, HIGH);
          delay(100);
          digitalWrite(UP_PIN, LOW);
          delay(100);
          digitalWrite(UP_PIN, HIGH);
          delay(100);

          delay(3000);
        }
      }
    }
  }
}

void handleSettings() {
  Serial.println("Settings");
  if (server.method() == HTTP_POST) {
    Serial.println("Got settings from browser");
    Serial.println(server.arg("plain"));
    File configFile = SPIFFS.open("/config.txt", "w");
    configFile.print(server.arg("plain"));
    deserializeJson(config, server.arg("plain"));
    configFile.close();
    initTimeClientFromConfig();
    server.send(200, "text/plain", "OK");
  }
  else if (server.method() == HTTP_GET) {
    Serial.println("Sent settings to the browser");
    String output;
    Serial.println("Sending currentTimeMS " + timeClient.getFormattedTime());
    DynamicJsonDocument configCopy(1024);
    configCopy = config;
    configCopy["currentTime"] = timeClient.getFormattedTime();
    serializeJson(configCopy, output);
    server.send(200, "text/json", output);
    Serial.println(output);
  }
}

void handleUp() {
  bool pressed = server.arg(0) == "true"; 
  if (pressed) {
    desiredDirection = 1;
    upLatched = true;
  } else {
    desiredDirection = 0;
    upLatched = false;
  }
  server.send(200, "text/plain", "OK");
}

void handleDown() {
  bool pressed = server.arg(0) == "true"; 
  if (pressed) {
    desiredDirection = -1;
    downLatched = true;
  } else {
    desiredDirection = 0;
    downLatched = false;
  }
  server.send(200, "text/plain", "OK");
}

void handleRoot() {
  handleFileRead("/index.html");
}

void handleJS() {
  handleFileRead("/blinds.js");
}

void handleCSS() {
  handleFileRead("/blinds.css");
}

String getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";         // If a folder is requested, send the index file
  String contentType = getContentType(path);            // Get the MIME type
  if (SPIFFS.exists(path)) {                           // If the file exists
    File file = SPIFFS.open(path, "r");                 // Open it
    size_t sent = server.streamFile(file, contentType); // And send it to the client
    file.close();                                       // Then close the file again
    return true;
  }
  Serial.println("\tFile Not Found");
  return false;                                         // If the file doesn't exist, return false
}
