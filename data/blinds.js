$(document).ready(function() {

    $("#upButton").mousedown(function() {
        onUpButtonPressed();
    });
    $("#upButton").on('touchstart', function(){
        onUpButtonPressed();
    });
    $("#upButton").mouseup(function() {
        onUpButtonReleased();
    });
    $("#upButton").on('touchend', function(){
        onUpButtonReleased();
    });
    $("#downButton").mousedown(function() {
        onDownButtonPressed();
    });
    $("#downButton").on('touchstart', function(){
        onDownButtonPressed();
    });
    $("#downButton").mouseup(function() {
       onDownButtonReleased();
    });
    $("#downButton").on('touchend', function(){
        onDownButtonReleased();
    });

    setupMiddlePanes();
});

const daysOfWeek = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"];
function setupMiddlePanes() {
    const middleContainer = $("div.middlePane");
    middleContainer.empty();
    for (const dayOfWeek of daysOfWeek) {
        const innerContainer = $("<div></div>");
        innerContainer.append("<span>" + dayOfWeek + "</span>");
        innerContainer.append("<input type='time' dow='" + dayOfWeek + "'></input>");
        middleContainer.append(innerContainer);
    }

    $.get('/settings', function(settings) {
        if (settings && settings.schedule) {
            for (const dow of Object.keys(settings.schedule)) {
                const input = $("div.middlePane input[dow=" + dow + "]");
                const value = settings.schedule[dow];
                if (value) {
                    let valueString = '';
                    if (value.localHours <= 9) {
                        valueString += '0';
                    }
                    valueString += value.localHours;
                    valueString += ":";
                    if (value.localMinutes <= 9) {
                        valueString += '0';
                    }
                    valueString += value.localMinutes;
                    input.val(valueString);
                } else {
                    input.val(null);
                }
            }

            $("div.timezonePane select option[timeZoneId='"
             + settings.timezoneId + "'] ").prop("selected", true);

            $("#dstCheckbox").prop("checked", settings.dst);
            $("#servertTime").text("Controller time is " + settings.currentTime);
        }
        attachHandlers();
        
    });
}

function attachHandlers() {
    const inputs = $("div.middlePane input");
    inputs.change(function() {
        sendSettings();
    });
    $("div.timezonePane select").change(function() {
        sendSettings();
    });
    $("#dstCheckbox").change(function() {
        sendSettings();
    });
}

function sendSettings() {
    const inputs = $("div.middlePane input");
    const settings = { schedule: {}};
    for (const input of inputs) {
        const key = $(input).attr("dow");
        const date = input.valueAsDate;
        let value = null;
        if (date) {
            value = {
                localHours: date.getUTCHours(),
                localMinutes: date.getUTCMinutes()
            };
        }
        settings.schedule[key] = value;
    }
    settings.offsetHours = parseInt($("div.timezonePane select").children("option:selected").val());
    settings.timezoneId = parseInt($("div.timezonePane select").children("option:selected").attr('timeZoneId'));
    settings.dst = $("#dstCheckbox").prop("checked") == true;
        
    postJson("/settings", settings);
}

function onUpButtonPressed() {
    $("#upButton").addClass("pressed");
    postData("/up", {pressed: true});
}

function onUpButtonReleased() {
    $("#upButton").removeClass("pressed");
    postData("/up", {pressed: false});
}

function onDownButtonPressed() {
    $("#downButton").addClass("pressed");
    postData("/down", {pressed: true});
}

function onDownButtonReleased() {
    $("#downButton").removeClass("pressed");
    postData("/down", {pressed: false});
}

function postData(url, data) {
    $.post(url, data);
}

function postJson(url, data) {
    if(typeof data == 'object') {
        data = JSON.stringify(data);
    }
    $.post(url, data, null, 'json');
}