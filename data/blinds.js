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

    setupMiddlePane();
});

const daysOfWeek = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"];
function setupMiddlePane() {
    const container = $("div.middlePane");
    container.empty();
    for (const dayOfWeek of daysOfWeek) {
        const innerContainer = $("<div></div>");
        innerContainer.append("<span>" + dayOfWeek + "</span>");
        innerContainer.append("<input type='time' dow='" + dayOfWeek + "'></input>");
        container.append(innerContainer);
    }

    $.get('/settings', function(settings) {
        if (settings) {
            for (const dow of Object.keys(settings)) {
                const input = $("div.middlePane input[dow=" + dow + "]");
                const value = settings[dow];
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
        }
        attachInputHandlers();
        
    });
}

function attachInputHandlers() {
    const inputs = $("div.middlePane input");
    inputs.change(function() {
        const settings = {};
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
            settings[key] = value;
        }
        postJson("/settings", settings);
    });
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