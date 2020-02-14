function getJSON(url, callback) {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', url, true);
    xhr.responseType = 'json';
    xhr.onload = function() {
        var status = xhr.status;
        if (status === 200) {
            callback(null, xhr.response);
        } else {
            callback(status, xhr.response);
        }
    };
    xhr.send();
};

function refreshValues() {
    getJSON('/json', function(err, data) {
        if(err !== null) {
            displayMessage('Error refreshing values!');
            console.log('Error retrieving JSON!');
            return;
        }
        var temp1Elem = document.getElementById('spanTemperature1');
        var fanControlElem = document.getElementById('spanFanControl');
        var fanControlEnabledElem = document.getElementById('checkAutomaticFanControlEnabled');
        var fanSpeed1Elem = document.getElementById('spanFanSpeed1');
        var fanSpeed2Elem = document.getElementById('spanFanSpeed2');
        var fanSpeed3Elem = document.getElementById('spanFanSpeed3');

        fanControlElem.innerHTML = (data.automaticFanControlEnabled ? 'on' : 'off') + (data.automaticFanControlEnabled ? ' (Set ' + (parseInt(data.activeFanControlSet) + 1) + ')' : '');
        fanControlEnabledElem.checked = data.automaticFanControlEnabled;
        temp1Elem.innerHTML = data.ntcTemperature;
        fanSpeed1Elem.innerHTML = data.fanSpeedsActive[0];
        fanSpeed2Elem.innerHTML = data.fanSpeedsActive[1];
        fanSpeed3Elem.innerHTML = data.fanSpeedsActive[2];

        var divValues = document.getElementById('divValues');
        divValues.classList.add('fade-out');
        setTimeout(function() { divValues.classList.remove('fade-out'); }, 100);

    });
}


var refreshInterval;
function setRefreshInterval(interval) {
    clearInterval(refreshInterval);
    if(typeof interval === 'undefined') { // get interval from radio buttons
        var form = document.getElementById('settingsForm');
        interval = parseInt(form.elements['refreshInterval'].value) * 1000; // seconds to ms
    }
    if (interval > 0) {
        refreshInterval = setInterval(refreshValues, interval < 100 ? interval * 1000 : interval); // avoid accidental input of seconds
    }
    console.log('set refresh interval: ', interval, 'ms');
}

function submitSettingsForm() {
    document.getElementById('settingsForm').submit();
}

function displayMessage(text) {
    console.log(text);
    var divMessageWindow = document.getElementById('divMessageWindow');
    divMessageWindow.innerHTML = text;

    // show message
    divMessageWindow.style.visibility = 'visible';
    divMessageWindow.style.opacity = '1';

    // fade out and hide
    setTimeout(function() { divMessageWindow.style.opacity = '0'; }, 2000);
    setTimeout(function() { divMessageWindow.style.visibility = 'hidden';}, 3000);
}

var fanControlSetRowTextTemplate = '<strong>#{{rowNumberText}}</strong>&nbsp;';
var fanControlSetRowTemplate = '<li data-row-number="{{rowNumber}}" id="fanControlSetRow{{rowNumber}}"><span id="fanControlSetRowText{{rowNumber}}">{{rowText}}</span><input type="text" size="2" placeholder="{{placeholderFan1}}" name="{{nameFan1}}" id="{{idFan1}}" value="{{valueFan1}}" />% <input type="text" size="2" placeholder="{{placeholderFan2}}" name="{{nameFan2}}" id="{{idFan2}}" value="{{valueFan2}}" />% <input type="text" size="2" placeholder="{{placeholderFan3}}" name="{{nameFan3}}" id="{{idFan3}}" value="{{valueFan3}}" />% above <input type="text" size="2" placeholder="{{placeholderTemp}}" name="{{nameTemp}}" id="{{idTemp}}" value="{{valueTemp}}" /> &deg;C <input type="button" onclick="removeFanControlSetRow(this.parentNode);" title="Remove row" class="link bold" value="&times;"/></li>';
var fanControlSetFirstRowTemplate = '<li id="fanControlSetRow{{rowNumber}}"><span id="fanControlSetRowText{{rowNumber}}">{{rowText}}</span><input type="text" size="2" placeholder="{{placeholderFan1}}" name="{{nameFan1}}" id="{{idFan1}}" value="{{valueFan1}}" />% <input type="text" size="2" placeholder="{{placeholderFan2}}" name="{{nameFan2}}" id="{{idFan2}}" value="{{valueFan2}}" />% <input type="text" size="2" placeholder="{{placeholderFan3}}" name="{{nameFan3}}" id="{{idFan3}}" value="{{valueFan3}}" />% below';

function removeFanControlSetRow(rowElem) {
    if(rowElem !== null && typeof rowElem !== 'undefined') {
        var id = parseInt(rowElem.getAttribute('data-row-number'));
        rowElem.parentNode.removeChild(rowElem); // remove row

        var hiddenFanControlSetCount = document.getElementById('hiddenFanControlSetCount');
        var fanControlSetCount = parseInt(hiddenFanControlSetCount.value);

        // update (rename) all following rows
        for(var i = id + 1; i < fanControlSetCount; i++) {
            rowElem = document.getElementById('fanControlSetRow' + i);
            rowElem.setAttribute('data-row-number', i - 1);

            var rowTextElem = document.getElementById('fanControlSetRowText' + i);
            var template = fanControlSetRowTextTemplate;
            template = template.replace(/{{rowNumberText}}/g, i);
            rowTextElem.innerHTML = template;
            rowTextElem.id = 'fanControlSetRowText' + (i - 1);

            var tempElem = document.getElementById('textFanControlSetTemp' + i);
            tempElem.name = 'fanControlSetTemp' + (i - 1);
            tempElem.id = 'textFanControlSetTemp' + (i - 1);

            for(var fanIndex = 0; fanIndex < 3; fanIndex++) {
                var fanElem = document.getElementById('textFanControlSetFanSpeed' + i + '_' + fanIndex);
                fanElem.name = 'fanControlSetFanSpeed' + (i - 1) + '_' + fanIndex;
                fanElem.id = 'textFanControlSetFanSpeed' + (i - 1) + '_' + fanIndex;
            }
            rowElem.id = 'fanControlSetRow' + (i - 1);

        }
        hiddenFanControlSetCount.value = fanControlSetCount - 1;
    }
}

function insertFanControlSetRow(values) {
    var hiddenFanControlSetCount = document.getElementById('hiddenFanControlSetCount');
    var fanControlSetCount = parseInt(hiddenFanControlSetCount.value || 0);
    var lastFanControlSetTempElem = document.getElementById('textFanControlSetTemp' + (fanControlSetCount - 1));
    var lastFanControlSetTemp = '';

    if(lastFanControlSetTempElem) {
        lastFanControlSetTemp = parseInt(lastFanControlSetTempElem.value);
    }

    var template;

    if(fanControlSetCount >= 9) {
        alert('Maximum count of control sets reached, behave!');
        return;
    }

    hiddenFanControlSetCount.value = fanControlSetCount + 1;

    if(fanControlSetCount === 0) {
        template = fanControlSetFirstRowTemplate;
    }
    else {
        template = fanControlSetRowTemplate;

        var placeholder = 'Temp';
        var name = 'fanControlSetTemp' + fanControlSetCount;
        var id = 'textFanControlSetTemp' + fanControlSetCount;
        var value = values ? values.tempThreshold : lastFanControlSetTemp + 1;

        template = template.replace(/{{placeholderTemp}}/g, placeholder);
        template = template.replace(/{{nameTemp}}/g, name);
        template = template.replace(/{{idTemp}}/g, id);
        template = template.replace(/{{valueTemp}}/g, value);
    }

    for(var fanIndex = 0; fanIndex < 3; fanIndex++) {
        var fanNumber = fanIndex + 1;
        var placeholder = 'Fan ' + fanNumber;
        var name = 'fanControlSetFanSpeed' + fanControlSetCount + '_' + fanIndex;
        var id = 'textFanControlSetFanSpeed' + fanControlSetCount + '_' + fanIndex;
        var value = values ? values.fanSpeeds[fanIndex] : '100'; // 100 percent as default value

        var textTemplate = fanControlSetRowTextTemplate;
        var regex = new RegExp('{{rowText}}', 'g');
        textTemplate = textTemplate.replace(regex, textTemplate);
        template = template.replace(/{{rowText}}/g, fanControlSetRowTextTemplate)
        regex = new RegExp('{{placeholderFan' + fanNumber + '}}', 'g');
        template = template.replace(regex, placeholder);
        regex = new RegExp('{{nameFan' + fanNumber + '}}', 'g');
        template = template.replace(regex, name);
        regex = new RegExp('{{idFan' + fanNumber + '}}', 'g');
        template = template.replace(regex, id);
        regex = new RegExp('{{valueFan' + fanNumber + '}}', 'g');
        template = template.replace(regex, value);
    }

    template = template.replace(/{{rowNumber}}/g, fanControlSetCount);
    template = template.replace(/{{rowNumberText}}/g, fanControlSetCount + 1);

    var divFanControlSets = document.getElementById('divFanControlSets');
    divFanControlSets.innerHTML += template;
}

function redrawFanControlSetRows(fanControlSets) {
    var divFanControlSets = document.getElementById('divFanControlSets');
    var hiddenFanControlSetCount = document.getElementById('hiddenFanControlSetCount');

    divFanControlSets.innerHTML = ''; // remove all rows
    hiddenFanControlSetCount.value = 0;

    if(fanControlSets && fanControlSets.length) {
        for(var fanControlSet of fanControlSets) {
            insertFanControlSetRow(fanControlSet);
        }
    }
    else { // always insert empty first row
        insertFanControlSetRow();
    }
}

// https://stackoverflow.com/questions/799981/document-ready-equivalent-without-jquery
function ready(callback){
    // in case the document is already rendered
    if (document.readyState !== 'loading') callback();
    // modern browsers
    else if (document.addEventListener) document.addEventListener('DOMContentLoaded', callback);
    // IE <= 8
    else document.attachEvent('onreadystatechange', function(){
        if (document.readyState === 'complete') callback();
    });
}

ready(function() {
    console.log('Loading JSON settings...');
    // on page load, get settings from json and insert values into html
    getJSON('/json', function(err, data) {
        if(err !== null) {
            displayMessage('Error loading settings from JSON!');
            return;
        }

        var checkDisplayEnabled = document.getElementById('checkDisplayEnabled');
        checkDisplayEnabled.checked = data.displayEnabled;
        var checkDisplayFlipScreen = document.getElementById('checkDisplayFlipScreen');
        checkDisplayFlipScreen.checked = data.displayFlipScreen;
        var checkAutomaticFanControlEnabled = document.getElementById('checkAutomaticFanControlEnabled');
        checkAutomaticFanControlEnabled.checked = data.automaticFanControlEnabled;
            
        redrawFanControlSetRows(data.fanControlSets);

        var textDisplayAddress = document.getElementById('textDisplayAddress');
        textDisplayAddress.value = '0x' + data.displayAddress.toString(16).toUpperCase(); // display hex address

        var textDisplayDurationPerMinute = document.getElementById('textDisplayDurationPerMinute');
        textDisplayDurationPerMinute.value = data.displayDurationPerMinute;

        var refreshIntervalElems = document.getElementsByName('refreshInterval');
        for(var i = 0; i < refreshIntervalElems.length; i++) {
            if(parseInt(refreshIntervalElems[i].value) === data.pageRefreshTime) {
                refreshIntervalElems[i].checked = true;
            }
        }
        setRefreshInterval(data.pageRefreshTime * 1000);
        displayMessage('Settings loaded!');
    });
});