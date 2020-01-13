var getJSON = function(url, callback) {
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
    //console.log('refresh');
    getJSON('/json', function(err, data) {
    	if(err !== null) {
    		displayMessage('Error refreshing values!');
    		console.log('Error retrieving JSON!'); return;
    	}
	    var temp1Elem = document.getElementById('temperature1');
	    var fanControlElem = document.getElementById('fanControl');
	    var fanControlEnabledElem = document.getElementById('automaticFanControlActive');
	    var fan1Elem = document.getElementById('fan1');
	    var fan2Elem = document.getElementById('fan2');
	    var fan3Elem = document.getElementById('fan3');

	    fanControlElem.innerHTML = (data.AutomaticFanControlActive ? 'on' : 'off') + (data.ActiveFanControlSet != 0 ? ' (Set ' + data.ActiveFanControlSet + ')' : '');
	    fanControlEnabledElem.checked = data.AutomaticFanControlActive;
    	temp1Elem.innerHTML = data.Temperature1;
    	fan1Elem.innerHTML = data.FanSpeedActive1;
    	fan2Elem.innerHTML = data.FanSpeedActive2;
    	fan3Elem.innerHTML = data.FanSpeedActive3;

    	var divValues = document.getElementById('divValues');
    	divValues.classList.add('fade-out');
    	setTimeout(function() { divValues.classList.remove('fade-out'); }, 100);

    });
}


var refreshInterval;
function setRefreshInterval(interval) {
    clearInterval(refreshInterval);
    if(typeof interval === 'undefined') {
		var form = document.getElementById('settingsForm');
    	interval = parseInt(form.elements['refreshInterval'].value) * 1000;
    }
    if (interval > 0) {
        refreshInterval = setInterval(refreshValues, interval);
    }
    console.log('refresh timer: ', interval, 'ms');
}

function submitSettingsForm() {
	document.getElementById('settingsForm').submit();
}

function displayMessage(text) {
	var messageWindow = document.getElementById('messageWindow');
	messageWindow.innerHTML = text;

 	messageWindow.style.visibility = 'visible';
	messageWindow.style.opacity = '1';
	setTimeout(function() { messageWindow.style.opacity = '0'; }, 2000);
	setTimeout(function() { messageWindow.style.visibility = 'hidden';}, 3000);
}

window.onload = function() {
	getJSON('/json', function(err, data) {
		if(err !== null) {
			displayMessage('Error loading settings!');
			console.log('Error retrieving initial JSON!'); return;
		}

	    var displayEnabledElem = document.getElementById('displayEnabled');
    	displayEnabledElem.checked = data.DisplayEnabled;
	    var displayFlipScreenElem = document.getElementById('displayFlipScreen');
    	displayFlipScreenElem.checked = data.DisplayFlipScreen;
	    var fanControlEnabledElem = document.getElementById('automaticFanControlActive');
	    fanControlEnabledElem.checked = data.AutomaticFanControlActive;

	    var fan1_1Elem = document.getElementById('fan1_1');
	    var fan1_2Elem = document.getElementById('fan1_2');
	    var fan1_3Elem = document.getElementById('fan1_3');
	    var fan1_4Elem = document.getElementById('fan1_4');
	    var fan2_1Elem = document.getElementById('fan2_1');
	    var fan2_2Elem = document.getElementById('fan2_2');
	    var fan2_3Elem = document.getElementById('fan2_3');
	    var fan2_4Elem = document.getElementById('fan2_4');
	    var fan3_1Elem = document.getElementById('fan3_1');
	    var fan3_2Elem = document.getElementById('fan3_2');
	    var fan3_3Elem = document.getElementById('fan3_3');
	    var fan3_4Elem = document.getElementById('fan3_4');
	    var temp_1Elem = document.getElementById('temp_1');
	    var temp_2Elem = document.getElementById('temp_2');
	    var temp_3Elem = document.getElementById('temp_3');
	    //var temp_4Elem = document.getElementById('temp_4');

	    fan1_1Elem.value = data.FanControlFan1_1;
	    fan1_2Elem.value = data.FanControlFan1_2;
	    fan1_3Elem.value = data.FanControlFan1_3;
	    fan1_4Elem.value = data.FanControlFan1_4;
	    fan2_1Elem.value = data.FanControlFan2_1;
	    fan2_2Elem.value = data.FanControlFan2_2;
	    fan2_3Elem.value = data.FanControlFan2_3;
	    fan2_4Elem.value = data.FanControlFan2_4;
	    fan3_1Elem.value = data.FanControlFan3_1;
	    fan3_2Elem.value = data.FanControlFan3_2;
	    fan3_3Elem.value = data.FanControlFan3_3;
	    fan3_4Elem.value = data.FanControlFan3_4;
	    temp_1Elem.value = data.FanControlTemp_1;
	    temp_2Elem.value = data.FanControlTemp_2;
	    temp_3Elem.value = data.FanControlTemp_3;
	    //temp_4Elem.value = data.FanControlTemp_4;

	    var displayAddressElem = document.getElementById('displayAddress');
	    displayAddressElem.value = '0x' + data.DisplayAddress.toString(16).toUpperCase();

	    var displayDurationPerMinuteElem = document.getElementById('displayDurationPerMinute');
	    displayDurationPerMinuteElem.value = data.DisplayDurationPerMinute;

	    var refreshIntervalElems = document.getElementsByName('refreshInterval');
	    for(var i = 0; i < refreshIntervalElems.length; i++) {
	    	if(refreshIntervalElems[i].value == data.PageRefreshTime) {
	    		refreshIntervalElems[i].checked = true;
	    	}
	    }
	    setRefreshInterval(data.PageRefreshTime * 1000);
	    console.log('Settings applied.');
	    displayMessage('Settings loaded!');
	});
};