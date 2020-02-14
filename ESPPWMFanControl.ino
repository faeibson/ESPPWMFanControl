/*
 * 
 * ESP PWM Fan Control 1.0 beta - © 2020 Fabian Brain. https://github.com/faeibson 
 * Thanks to all you guys contributing all these awesome libraries to the community!
 * 
 * This project is licensed unter the GNU General Public License v3.0
 */

#include "ESPPWMFanControl.h"

// the "fan channel" to control all fans simultaneously
#define FANS_ALL        -1

ESP8266WebServer *webServer;
//PubSubClient *mqttClient;
SSD1306Brzo *display;
Storage *storage;

void setup()
{
    // setup serial port
    Serial.begin(115200);
    Serial.println(F("ESP PWM Fan Control starting...\n"));

    
    // setup PWM pins
    for(uint8_t i = 0; i < settings::fanCount; i++) {
        pinMode(settings::fanPins[i], OUTPUT);
    }
    analogWriteRange(settings::pwmValueRange);  
    analogWriteFreq(settings::pwmFrequency);

    
    // delete settings file on startup
    //SPIFFS.begin();
    //SPIFFS.remove(settings::settingsJsonFileName);

    
    // initialize configuration storage
    storage = new Storage(settings::settingsJsonFileName, settings::fanPins, settings::fanCount, settings::pwmValueRange, settings::maxFanControlSets, settings::debug);
    storage->load();

    
    // configure wifi
    WiFiManager *wifiManager = new WiFiManager();
    wifiManager->autoConnect(storage->id);
    Serial.println(F("WiFi connected!"));

    
    // configure MQTT
    //mqttClient = new PubSubClient(client);
    //setup_mqtt();
    //Serial.println(F("MQTT connected!"));


    // configure web server
    webServer = new ESP8266WebServer(80);
    webServer->begin();

    webServer->on(PSTR("/"), handleRoot);
    webServer->on(PSTR("/fans/on"), handleFansOn);
    webServer->on(PSTR("/fans/off"), handleFansOff);
    webServer->on(PSTR("/fans/set"), handleFansSet);
    webServer->on(PSTR("/json"), handleJson);
    webServer->on(PSTR("/settings/save"), handleSettingsSave);
    webServer->on(PSTR("/settings/resetAll"), handleSettingsResetAll);
    webServer->on(PSTR("/settings/reset"), handleSettingsReset);
    webServer->on(PSTR("/settings/restart"), handleSettingsRestart);
    webServer->on(PSTR("/download"), handleDownload);
    
    //webServer->serveStatic(PSTR("/static/style.css"), SPIFFS, settings::cssFileName);
    //webServer->serveStatic(PSTR("/static/script.js"), SPIFFS, settings::javaScriptFileName);
    //webServer->serveStatic(PSTR("/static/favicon.ico"), SPIFFS, settings::faviconFileName, "max-age=86400");

    // serve assets from flash
    webServer->on(PSTR("/static/script.js"), []() {
        webServer->send_P(200, progmem_assets_strings::mime_application_javascript, progmem_assets_javascript::main);
    });
    webServer->on(PSTR("/static/style.css"), []() {
        webServer->send_P(200, progmem_assets_strings::mime_text_css, progmem_assets_css::main);
    });
    webServer->on(PSTR("/static/favicon.ico"), []() {
        webServer->send_P(200, progmem_assets_strings::mime_image_x_icon, progmem_assets_images::favicon, progmem_assets_images::favicon_len);
    });
    
    webServer->onNotFound(handleNotFound);
    Serial.println(F("WebServer up and running!"));


    // set initial fan speed
    if(!storage->automaticFanControlEnabled) {
        //setFanSpeed(storage->fanSpeedActive1, storage->fanSpeedActive2, storage->fanSpeedActive3, false, false);
        setFanSpeeds((int8_t*)storage->fanSpeedsActive, false, false);
    }

    // initialize display
    if(storage->displayEnabled) {
        initDisplay();
    }
  
    Serial.println(F("\nESP PWM Fan Control setup complete.\n"));
}

void loop()
{
    /*if (!mqttClient->connected() && strlen(storage->mqttBroker) > 3) { reconnect_mqtt(); }
      mqttClient->loop();*/
    webServer->handleClient(); // http client

    // read temp (and do the other stuff) once per second
    if((storage->ntcTemperatureTime + 1000) < millis()) {
        //storage->ntcTemperature = readTemperature();
        storage->ntcTemperature = (float)random(20, 30); // test mode
    
        if(storage->displayEnabled) {
            if(storage->displayIsOn && storage->displayDurationPerMinute != 60) {
                // display on-time per minute exceeded -> turn off
                if(storage->displayLastOnTime + storage->displayDurationPerMinute * 1000 < millis()) {
                    displayValues(false);
                }
            }

            // turn display on after one minute
            if(storage->displayLastOnTime == 0 || storage->displayLastOnTime + 60000 < millis()) {
                storage->displayLastOnTime = millis();
                displayValues(true);
            }
            // refresh every second if turned on
            else if(storage->displayIsOn) {
                displayValues(true);
            }
        }
        // turn display off if not enabled
        else if(storage->displayIsOn) {
            displayValues(false);
        }

        // do the fan control ;)
        if(storage->automaticFanControlEnabled) {
            fanControl();
        }
        else {
            storage->activeFanControlSet = 0;
        }
    
        Serial.print(F("Temperature: ")); Serial.println(storage->ntcTemperature);
        storage->ntcTemperatureTime = millis();
        
        if(settings::debug) {
            Serial.println(F("\nESP Diagnosis: "));
            Serial.print(F("Free heap: ")); Serial.println(ESP.getFreeHeap());
            Serial.print(F("Heap fragmentation: ")); Serial.println(ESP.getHeapFragmentation());
            Serial.print(F("Max free block size: ")); Serial.println(ESP.getMaxFreeBlockSize());
            Serial.println();
        }
    }

    // reset the esp if desired...
    if(storage->resetESP) {
        if(millis() > storage->resetESPTime) {
            Serial.println(F("=====\nRESETTING ESP...\n====="));
            storage->resetESP = false;
            WiFi.disconnect(true);
            ESP.reset();
        }
    }
}

void handleSettingsSave() {
    if(webServer->args() == 0) {
        handleRedirect("/");
        return;
    }
    uint8_t refreshInterval = webServer->arg("refreshInterval").toInt();
    uint8_t displayDurationPerMinute = webServer->arg("displayDurationPerMinute").toInt();
    bool displayEnabled = webServer->arg("displayEnabled") == "1";
    bool displayFlipScreen = webServer->arg("displayFlipScreen") == "1";
    bool automaticFanControlEnabled = webServer->arg("automaticFanControlEnabled") == "1";
    uint8_t fanControlSetCount = min(storage->maxFanControlSets, (uint8_t)webServer->arg("fanControlSetCount").toInt()); // limit set count

    /*
    uint8_t fan1_1 = min(max(0, (uint8_t)(webServer->arg("fan1_1").toInt() / FAN_PERCENT_STEP_SIZE) * FAN_PERCENT_STEP_SIZE), 100);
    uint8_t fan1_2 = min(max(0, (uint8_t)(webServer->arg("fan1_2").toInt() / FAN_PERCENT_STEP_SIZE) * FAN_PERCENT_STEP_SIZE), 100);
    uint8_t fan1_3 = min(max(0, (uint8_t)(webServer->arg("fan1_3").toInt() / FAN_PERCENT_STEP_SIZE) * FAN_PERCENT_STEP_SIZE), 100);
    uint8_t fan1_4 = min(max(0, (uint8_t)(webServer->arg("fan1_4").toInt() / FAN_PERCENT_STEP_SIZE) * FAN_PERCENT_STEP_SIZE), 100);
    uint8_t fan2_1 = min(max(0, (uint8_t)(webServer->arg("fan2_1").toInt() / FAN_PERCENT_STEP_SIZE) * FAN_PERCENT_STEP_SIZE), 100);
    uint8_t fan2_2 = min(max(0, (uint8_t)(webServer->arg("fan2_2").toInt() / FAN_PERCENT_STEP_SIZE) * FAN_PERCENT_STEP_SIZE), 100);
    uint8_t fan2_3 = min(max(0, (uint8_t)(webServer->arg("fan2_3").toInt() / FAN_PERCENT_STEP_SIZE) * FAN_PERCENT_STEP_SIZE), 100);
    uint8_t fan2_4 = min(max(0, (uint8_t)(webServer->arg("fan2_4").toInt() / FAN_PERCENT_STEP_SIZE) * FAN_PERCENT_STEP_SIZE), 100);
    uint8_t fan3_1 = min(max(0, (uint8_t)(webServer->arg("fan3_1").toInt() / FAN_PERCENT_STEP_SIZE) * FAN_PERCENT_STEP_SIZE), 100);
    uint8_t fan3_2 = min(max(0, (uint8_t)(webServer->arg("fan3_2").toInt() / FAN_PERCENT_STEP_SIZE) * FAN_PERCENT_STEP_SIZE), 100);
    uint8_t fan3_3 = min(max(0, (uint8_t)(webServer->arg("fan3_3").toInt() / FAN_PERCENT_STEP_SIZE) * FAN_PERCENT_STEP_SIZE), 100);
    uint8_t fan3_4 = min(max(0, (uint8_t)(webServer->arg("fan3_4").toInt() / FAN_PERCENT_STEP_SIZE) * FAN_PERCENT_STEP_SIZE), 100);
    float temp_1 = webServer->arg("temp_1").toFloat();
    float temp_2 = webServer->arg("temp_2").toFloat();
    float temp_3 = webServer->arg("temp_3").toFloat();
    float temp_4 = webServer->arg("temp_4").toFloat();
  
    storage->fanControlFan1_1 = fan1_1;
    storage->fanControlFan1_2 = fan1_2;
    storage->fanControlFan1_3 = fan1_3;
    storage->fanControlFan1_4 = fan1_4;
    storage->fanControlFan2_1 = fan2_1;
    storage->fanControlFan2_2 = fan2_2;
    storage->fanControlFan2_3 = fan2_3;
    storage->fanControlFan2_4 = fan2_4;
    storage->fanControlFan3_1 = fan3_1;
    storage->fanControlFan3_2 = fan3_2;
    storage->fanControlFan3_3 = fan3_3;
    storage->fanControlFan3_4 = fan3_4;
    storage->fanControlTemp_1 = temp_1;
    storage->fanControlTemp_2 = temp_2;
    storage->fanControlTemp_3 = temp_3;
    storage->fanControlTemp_4 = temp_4;*/

    /*delete [] storage->fanControlSets;
    storage->fanControlSets = new FanControlSet[fanControlSetCount];*/
    for(uint8_t i = 0; i < storage->fanControlSetCount; i++) {
        delete storage->fanControlSets[i];
    }
    
    storage->fanControlSetCount = fanControlSetCount;
    storage->fanControlSets = new FanControlSet*[fanControlSetCount];

    for(uint8_t i = 0; i < fanControlSetCount; i++) {
        storage->fanControlSets[i] = new FanControlSet(settings::fanCount);
        //for(uint8_t j = 0; j < (sizeof(storage->fanControlSets[0].fanSpeeds) / sizeof(uint8_t)); j++) {
        for(uint8_t j = 0; j < settings::fanCount; j++) {
            // min 0, max 100, fit to calculated PWM steps
            uint8_t fanSpeed = storage->normalizeFanSpeedValue((uint8_t)webServer->arg(String("fanControlSetFanSpeed") + i + "_" + j).toInt());
            storage->fanControlSets[i]->fanSpeeds[j] = fanSpeed;
        }
        
        String fanControlSetTempArg = webServer->arg(String("fanControlSetTemp") + i);
        /*float fanControlSetTemp = std::numeric_limits<float>::min();
        if(fanControlSetTempArg != "") {
            fanControlSetTemp = fanControlSetTempArg.toFloat();
        }*/
        storage->fanControlSets[i]->tempThreshold = fanControlSetTempArg.toFloat();
    }
    storage->sortFanControlSets();
  
    String displayAddressString = webServer->arg("displayAddress");
    uint8_t displayAddress;
    if(displayAddressString == "") { // fall back
      displayAddress = 0x3C;
    }
    else {
      displayAddress = FanControlHelper::convertHexStringToInt((char*)displayAddressString.c_str());
    }
  
    storage->pageRefreshTime = refreshInterval;
  
    storage->displayAddress = displayAddress;
    storage->displayDurationPerMinute = max(0, min(60, (int)displayDurationPerMinute));
    storage->displayEnabled = displayEnabled;
    storage->displayFlipScreen = displayFlipScreen;
    /*if(storage->automaticFanControlEnabled && !automaticFanControlEnabled) {
        setFanSpeed(storage->fanSpeed1, storage->fanSpeed2, storage->fanSpeed3); // restore previous manual setting
    }*/
    storage->automaticFanControlEnabled = automaticFanControlEnabled;
  
    storage->displayLastOnTime = 0; // always reset display timeout
  
    storage->save();
    initDisplay();
    handleRootWithMessage(F("Settings saved."));
}

void handleFansOn() {
    setFanSpeeds((int8_t*)storage->fanSpeeds, false, true);
    
    String content = String(F("Fans turned on: "));
    for(uint8_t i = 0; i < storage->fanCount; i++) {
        content += String(storage->fanSpeedsActive[i]);
        if(i < storage->fanCount - 1) {
            content += F("% / ");
        }
        else {
            content += F("%");
        }
    }
    
    handleRootWithMessage(content);
}

void handleFansOff() {
    setFanSpeed(FANS_ALL, 0, false, true);
    storage->save();
    String content = F("Fans turned off.<br />");
    handleRootWithMessage(content);
}

void handleFansSet() {
    int8_t fanIndex = webServer->arg("fan").toInt() - 1; // number to index, 0 to -1
    uint8_t fanSpeed = webServer->arg("speed").toInt();

    if(settings::debug) {
        Serial.print(F("Set fan ")); Serial.print(fanIndex); Serial.print(F(" to ")); Serial.print(fanSpeed); Serial.println(F("%"));
    }
    setFanSpeed(fanIndex, fanSpeed, true, true);
    storage->save();
  
    String content = String(F("Fans set to: "));
    for(uint8_t i = 0; i < storage->fanCount; i++) {
        content += String(storage->fanSpeedsActive[i]);
        if(i < storage->fanCount - 1) {
            content += F("% / ");
        }
        else {
            content += F("%");
        }
    }
    handleRootWithMessage(content);
}

void handleRoot() {
    String temporaryMessage = storage->getTemporaryMessage(); //webServer->arg("message");
    
    // HTML begin
    String contents = F("<!DOCTYPE html><html lang='en'><head><meta name='viewport' content='width=device-width, initial-scale=1' /><meta charset='UTF-8' />"
        "<title>Fan Control</title>"
        "<link rel='shortcut icon' href='/static/favicon.ico' />"
        "<link rel='stylesheet' href='/static/style.css' />"
        "<script type='text/javascript' src='/static/script.js'></script></head>");

/*
    // favicon
    contents += "";
    // CSS
    contents += "";
    // JavaScript
    contents += "";
*/

    // HTML body
    contents += String(F("<body><div class='container'>"
    
        // headline
                    "<div class='top'><h1><a href='/'>Fan Control</a></h1></div>"

        // begin of wrapper container, values container
                    "<div class='wrapper'><div class='content values' id='divValues'>"
                    "Temperature: <span id='spanTemperature1' class='fade'>")) + storage->ntcTemperature + F("</span> &deg;C<br/><br />"
                    "Auto fan control: <span id='spanFanControl' class='fade'>") + (storage->automaticFanControlEnabled ? String(F("on (Set ")) + String(storage->activeFanControlSet + 1) + F(")") : String(F("off"))) + F("</span><br/>");
    for(uint8_t i = 0; i < storage->fanCount; i++) {                 
        /*"Fan 1: <span id='spanFanSpeed1' class='fade'>") + storage->fanSpeedActive1 + F("</span> %<br />"
        "Fan 2: <span id='spanFanSpeed2' class='fade'>") + storage->fanSpeedActive2 + F("</span> %<br/>"
        "Fan 3: <span id='spanFanSpeed3' class='fade'>") + storage->fanSpeedActive3 + F("</span> %</div>"*/
        contents += String(F("Fan ")) + (i + 1) + F(": <span id='spanFanSpeed") + (i + 1) + F("' class='fade'>") + storage->fanSpeedsActive[i] + F("</span> %<br />");
    }

        // menu
    contents += String(F("</div><div class='menu'><ul class='nav'>"

            // fan control
                        "<li class='heading'>Fan control</li>"
                        "<li><a href='/fans/on'>Turn fans on (with last manual setting)</a></li>"
                        "<li><a href='/fans/off'>Turn fans off</a></li>"
                        "<li><a href='/fans/set?fan=0&speed=25'>Set all fans to 25%</a></li>"
                        "<li><a href='/fans/set?fan=0&speed=50'>Set all fans to 50%</a></li>"
                        "<li><a href='/fans/set?fan=0&speed=75'>Set all fans to 75%</a></li>"
                        "<li><a href='/fans/set?fan=0&speed=100'>Set all fans to 100%</a></li>"
                        "<li><form method='get' action='/fans/set'>Set fan <input type='text' name='fan' value='0' size='1' /> (0 = all) to <input name='speed' type='text' size='3' /> % <input type='submit' value='set' class='link' /></form></li>"));
    
    contents += String(F(
            // settings
                        "<form id='settingsForm' method='post' action='/settings/save'>"

                // automatic fan control
                            "<li class='heading'>Automatic fan control</li>"
                            "<li><label>Enabled: <input type='checkbox' id='checkAutomaticFanControlEnabled' name='automaticFanControlEnabled' value='1' onchange='submitSettingsForm()' /></label></li>"
                            "<input type='hidden' name='fanControlSetCount' id='hiddenFanControlSetCount' />"
                            "<div id='divFanControlSets'>"
                            "</div>"
                            "<li class='bold'><input type='button' onclick='insertFanControlSetRow()' title='Add new row' value='Add new row' /></li>"
                            "<li>The rows will be sorted by temperature after saving.</li>"
                    
                // auto refresh
                            "<li class='heading'>Auto refresh</li>"
                            "<li><label><input type='radio' name='refreshInterval' value='0' onclick='setRefreshInterval(0);submitSettingsForm()' /> off</label>&nbsp;<label><input type='radio' name='refreshInterval' value='1' onclick='setRefreshInterval();submitSettingsForm()' /> 1s</label>&nbsp;<label><input type='radio' name='refreshInterval' value='3' onclick='setRefreshInterval();submitSettingsForm()' /> 3s</label>&nbsp;<label><input type='radio' name='refreshInterval' value='5' onclick='setRefreshInterval();submitSettingsForm()' /> 5s</label>&nbsp;<label><input type='radio' name='refreshInterval' value='10' onclick='setRefreshInterval();submitSettingsForm()' checked='checked' /> 10s</label></li>"

                // display
                            "<li class='heading'>Display</li>"
                            "<li><label>Enabled: <input type='checkbox' name='displayEnabled' id='checkDisplayEnabled' value='1' /></label></li>"
                            "<li><label>Flip vertically: <input type='checkbox' name='displayFlipScreen' id='checkDisplayFlipScreen' value='1' /></label></li>"
                            "<li>Address: <input type='text' name='displayAddress' id='textDisplayAddress' value='0x3C' placeholder='0x3C' size='6' /></li>"
                            "<li>Display time per minute: <input type='text' name='displayDurationPerMinute' id='textDisplayDurationPerMinute' value='30' size='4' /> seconds<br /></li>"
                            "<li class='heading'><input type='button' class='link padding-10 color-secondary' onclick='submitSettingsForm()' value='Save settings'/></li>"
                        "</form>"
                    
            // general commands
                        "<li class='heading'>General</li>"
                        "<li class='bold'><a href='/json' target='_blank'>Show JSON info</a></li>"
                        "<li class='bold'><a href='/download?file=settings' target='_blank'>Download settings file</a></li>"
                        "<li class='bold'><a href='/settings/resetAll' onclick='if(!confirm(\"Confirm reset\")) return false;'>Reset settings + WiFi</a></li>"
                        "<li class='bold'><a href='/settings/reset' onclick='if(!confirm(\"Confirm reset\")) return false;'>Reset settings</a></li>"
                        "<li class='bold'><a href='/settings/restart' onclick='if(!confirm(\"Confirm restart\")) return false;'>Restart ESP</a></li>"
                    
        // menu end
                    "</ul></div></div>"

        // message window for status messages
                    "<div id='divMessageWindow' class='messageWindow'></div>"));
                    
    if(temporaryMessage != "") {
        contents += String(F("<div id='divTemporaryMessage' class='temporaryMessage'>")) + temporaryMessage + F("</div>");
    }
    
    if(settings::debug) {
        // show HTTP request info
        contents += String(F("<script>console.log('HTTP request:', 'URI: ")) + webServer->uri() + F("', 'Method: ") + ((webServer->method() == HTTP_GET) ? "GET" : "POST") + F("', 'Arguments: ") + webServer->args() + "');";
        for (uint8_t i = 0; i < webServer->args(); i++) {
            contents += "console.log('" + webServer->argName(i) + ":', '" + webServer->arg(i) + F("');");
        }
        contents += String(F("</script>"));
    }

    // HTML end
    contents += String(F("<br /><br /><br />"
                    "</div></body></html>"));

    // send HTTP response to client
    webServer->send(200, progmem_assets_strings::mime_text_html, contents);
}

void handleRootWithMessage(String message) {
    handleRedirect("/", message);
}

void handleRedirect(String location) {
    handleRedirect(location, "");
}

void handleRedirect(String location, String message) {
    if(message != "") {
        storage->setTemporaryMessage(message);
    }
    webServer->sendHeader(FPSTR(progmem_assets_strings::location), location, true);
    webServer->send_P(302, progmem_assets_strings::mime_text_plain, "");
}

void handleNotFound() {
    if(settings::debug) {
        Serial.print(F("Requested URI not found: ")); Serial.println(webServer->uri());
    }
    handleRedirect("/", "Page not found!");
}

void handleJson() {
    webServer->send(200, progmem_assets_strings::mime_application_json, storage->json());
}

void handleDownload() {
    String file = webServer->arg("file");
    if(settings::debug) {
        Serial.print(F("Download requested: ")); Serial.println(file);
    }
    if(file == F("settings")) {
        file = settings::settingsJsonFileName;
    }
    if(file == "" || !streamFile(file, true)) {
        webServer->send_P(404, progmem_assets_strings::mime_text_plain, "");
    }
}

void handleSettingsRestart() {
    handleRedirect("/");
    delay(1000);
    ESP.restart();
}

void handleSettingsReset() {
    SPIFFS.begin();
    SPIFFS.remove(settings::settingsJsonFileName);
    delete(storage);
    storage = new Storage(settings::settingsJsonFileName, settings::fanPins, settings::fanCount, settings::pwmValueRange, settings::maxFanControlSets, settings::debug);
    handleRedirect("/");
}

void handleSettingsResetAll() {
    webServer->send(200, progmem_assets_strings::mime_text_plain, F("Resetting the controller in 5 seconds... Wifi will have to be reconfigured, too!"));
    storage->resetESP = true;
    storage->resetESPTime = millis() + 5000;
}

uint8_t fanControl() {
    uint8_t activeFanControlSet = 0; // default to first row
    for(uint8_t i = storage->fanControlSetCount - 1; i >= 1; i--) {
        if(storage->ntcTemperature > storage->fanControlSets[i]->tempThreshold) {
            // control sets are ordered
            // beginning from the last, we can take the first one that applies
            activeFanControlSet = i;
            break;
        }
        // otherwise leave default
    }
    if(settings::debug) {
        Serial.print(F("Active fan control set: "));Serial.println(activeFanControlSet);
    }

    if(storage->activeFanControlSet != activeFanControlSet) {
        setFanSpeeds((int8_t*)storage->fanControlSets[activeFanControlSet]->fanSpeeds, false, false);
        storage->activeFanControlSet = activeFanControlSet;
        Serial.println(String(F("Auto fan control: Applying set ")) + storage->activeFanControlSet);
    }
    return activeFanControlSet;
}

/*void setFanSpeed(int8_t fan1, int8_t fan2, int8_t fan3, bool permanent, bool turnOffFanControl) {
    if (fan1 < 0) { // -1 => ignore
        fan1 = storage->fanSpeedActive1;
    }
    else if(permanent) {
        storage->fanSpeed1 = min((fan1 / FAN_PERCENT_STEP_SIZE) * FAN_PERCENT_STEP_SIZE, 100);
    }
    
    if (fan2 < 0) {
        fan2 = storage->fanSpeedActive2;
    }
    else if(permanent) {
        storage->fanSpeed2 = min((fan2 / FAN_PERCENT_STEP_SIZE) * FAN_PERCENT_STEP_SIZE, 100);
    }
    
    if (fan3 < 0) {
        fan3 = storage->fanSpeedActive3;
    }
    else if(permanent) {
        storage->fanSpeed3 = min((fan3 / FAN_PERCENT_STEP_SIZE) * FAN_PERCENT_STEP_SIZE, 100);
    }
  
    analogWrite(FAN_1_PIN, min(fan1 / FAN_PERCENT_STEP_SIZE, ANALOG_WRITE_RANGE));
    analogWrite(FAN_2_PIN, min(fan2 / FAN_PERCENT_STEP_SIZE, ANALOG_WRITE_RANGE));
    analogWrite(FAN_3_PIN, min(fan3 / FAN_PERCENT_STEP_SIZE, ANALOG_WRITE_RANGE));
  
    storage->fanSpeedActive1 = min((fan1 / FAN_PERCENT_STEP_SIZE) * FAN_PERCENT_STEP_SIZE, 100);
    storage->fanSpeedActive2 = min((fan2 / FAN_PERCENT_STEP_SIZE) * FAN_PERCENT_STEP_SIZE, 100);
    storage->fanSpeedActive3 = min((fan3 / FAN_PERCENT_STEP_SIZE) * FAN_PERCENT_STEP_SIZE, 100);
  
    if(turnOffFanControl) {
        storage->automaticFanControlEnabled = false;
    }
}*/

void setFanSpeeds(int8_t fanSpeeds[], bool permanent, bool turnOffFanControl) {
    for(uint8_t i = 0; i < sizeof(fanSpeeds) / sizeof(int8_t); i++) {
        if(fanSpeeds[i] < 0) { // -1 => ignore this one
            continue;
        }
        setFanSpeed(i, (uint8_t)fanSpeeds[i], permanent, turnOffFanControl);
    }
}

void setFanSpeed(int8_t fanIndex, uint8_t fanSpeed, bool permanent, bool turnOffFanControl) {
    if(fanIndex >= storage->fanCount || (fanIndex < 0 && fanIndex != FANS_ALL)) { // validate index
        if(settings::debug) {
            Serial.println(F("Invalid fan index!"));
        }
        return; 
    }
    if(fanIndex == FANS_ALL) { // set all fans to fanSpeed
        if(settings::debug) {
            Serial.print(F("Set all fans to ")); Serial.println(fanSpeed);
        }
        for(uint8_t i = 0; i < storage->fanCount; i++) {
            setFanSpeed(i, fanSpeed, permanent, turnOffFanControl);
        }
        return;
    }
    
    uint8_t value = storage->normalizeFanSpeedValue(fanSpeed); // adjust value to calculated steps limited by 100 percent
    if(permanent) {
        storage->fanSpeeds[fanIndex] = value;
    }

    if(settings::debug) {
        Serial.print(F("Set fan ")); Serial.print(fanIndex); Serial.print(F(" to ")); Serial.println(fanSpeed);
    }
    analogWrite(storage->fanPins[fanIndex], value);
    storage->fanSpeedsActive[fanIndex] = value;
    if(turnOffFanControl) {
        storage->automaticFanControlEnabled = false;
    }
}

/*void setFanSpeed(int8_t fan1, int8_t fan2, int8_t fan3) {
    setFanSpeed(fan1, fan2, fan3, true, true);
}

void setFanSpeed(FanControlSet fanControlSet) {
    setFanSpeed(fanControlSet.fanSpeeds[0], fanControlSet.fanSpeeds[1], fanControlSet.fanSpeeds[2], false, false);
}*/

void displayValues(bool turnOn) {
    if(!storage->displayIsOn && turnOn) {
        display->displayOn();
        if(settings::debug) {
            Serial.println(F("Display turned on."));
        }
    }
    else if(storage->displayIsOn && !turnOn) {
        display->displayOff();
        if(settings::debug) {
            Serial.println(F("Display turned off."));
        }
    }
  
    if(turnOn) {
        display->clear();
        
        if(storage->displayFlipScreen) {
            display->flipScreenVertically();
        }
    
        // the yellow top area is crap on the tested display
        /*display->setFont(Open_Sans_Regular_10);
        display->setTextAlignment(TEXT_ALIGN_LEFT);
        display->drawString(0, 2, String(F("Fan Control")));*/
        
        display->setFont(ArialMT_Plain_10);
        display->setTextAlignment(TEXT_ALIGN_LEFT);
        display->drawString(0, 23, String(F("Fan 1: ")) + storage->fanSpeedsActive[0] + " %");
        display->drawString(0, 37, String(F("Fan 2: ")) + storage->fanSpeedsActive[1] + " %");
        display->drawString(0, 51, String(F("Fan 3: ")) + storage->fanSpeedsActive[2] + " %");
    
        display->setFont(progmem_assets_fonts::Open_Sans_Bold_12);
        display->setTextAlignment(TEXT_ALIGN_RIGHT);
        display->drawString(117, 20, String(F("Temp:")));
        display->drawString(128, 36, String(storage->ntcTemperature) + F(" °C"));
        display->setFont(ArialMT_Plain_10);
        String activeSetString = "";
        if(storage->automaticFanControlEnabled) {
            activeSetString = String(F("Set ")) + (storage->activeFanControlSet + 1);
        }
        else {
            activeSetString = F("off");
        }
        display->drawString(128, 51, String(F("Auto: ")) + activeSetString);
        
        display->display();
    }
    storage->displayIsOn = turnOn;
}

bool streamFile(String filename, bool download) {
    SPIFFS.begin();
    String path = filename;
    if (path.endsWith(PSTR("/"))) {
        return false;
    }
    
    if(path[0] != '/') {
        path = "/" + path;
    }
    else {
        filename = filename.substring(1);
    }
    
    String contentType = getContentType(path, download);
    if(SPIFFS.exists(path)) {
        File file = SPIFFS.open(path, "r");

        webServer->sendHeader(PSTR("content-disposition"), String(F("attachment; filename=\"")) + filename + "\"");
        webServer->streamFile(file, contentType);
        file.close();
        return true;
    }
    return false;
}

float readTemperature() {
    return FanControlHelper::calculateNTCTemperature(analogRead(settings::ntcAnalogPin), settings::ntcValueRange, settings::ntcVoltage, settings::ntcReferenceResistance, settings::ntcReferenceTemperature, settings::ntcBeta, settings::ntcPullUpResistorValue, settings::ntcRoundTemperatureToDecimals);
}

void initDisplay() {
    if(display != NULL) {
        display->end();
        delete(display);
    }
    display = new SSD1306Brzo(storage->displayAddress, settings::sdaPin, settings::sclPin);
    display->init();
    Serial.println(F("Init display..."));
}

String getContentType(String filename, bool download) {
    const char *contentType;
    if (download) {
        contentType = progmem_assets_strings::mime_application_octet_stream;
    } else if (filename.endsWith(PSTR(".htm")) || filename.endsWith(PSTR(".html"))) {
        contentType = progmem_assets_strings::mime_text_html;
    } else if (filename.endsWith(PSTR(".css"))) {
        contentType = progmem_assets_strings::mime_text_css;
    } else if (filename.endsWith(PSTR(".js"))) {
        contentType = progmem_assets_strings::mime_application_javascript;
    } else if (filename.endsWith(PSTR(".json"))) {
        contentType = progmem_assets_strings::mime_application_json;
    } else if (filename.endsWith(PSTR(".png"))) {
        contentType = progmem_assets_strings::mime_image_png;
    } else if (filename.endsWith(PSTR(".gif"))) {
        contentType = progmem_assets_strings::mime_image_gif;
    } else if (filename.endsWith(PSTR(".jpg")) || filename.endsWith(PSTR(".jpeg"))) {
        contentType = progmem_assets_strings::mime_image_jpeg;
    } else if (filename.endsWith(PSTR(".ico"))) {
        contentType = progmem_assets_strings::mime_image_x_icon;
    } else if (filename.endsWith(PSTR(".xml"))) {
        contentType = progmem_assets_strings::mime_application_x_gzip;
    } else if (filename.endsWith(PSTR(".pdf"))) {
        contentType = progmem_assets_strings::mime_application_x_pdf;
    } else if (filename.endsWith(PSTR(".zip"))) {
        contentType = progmem_assets_strings::mime_application_x_zip;
    } else if (filename.endsWith(PSTR(".gz"))) {
        contentType = progmem_assets_strings::mime_application_x_gzip;
    } else {
        contentType = progmem_assets_strings::mime_text_plain;
    }
    return FPSTR(contentType);
}

/*void handleMqttMessage(char* topic, byte* payload, unsigned int length) {
    Serial.print("MQTT Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    char message[length + 1];
    ByteToChar(payload, message, length);
    Serial.println(message);
  
    if (!strcmp(topic, "ESPFanControl/on")) {
        Serial.println("MQTT on");
        setFanSpeed(storage->FanSpeed1, storage->FanSpeed2, storage->FanSpeed3);
    }
  
    if (!strcmp(topic, "ESPFanControl/off")) {
        Serial.println("MQTT off");
        setFanSpeed(0, 0, 0);
    }
}

void setup_mqtt() {
      mqttClient->setServer(storage->mqttBroker, storage->mqttPort);
      mqttClient->setCallback(handleMqttMessage);
  }

void reconnect_mqtt() {
    while (!mqttClient->connected() && mqttRetriesLeft-- > 0) {
        Serial.print(storage->mqttBroker);
        Serial.println("...");
        bool connected = false;
        if(storage->mqttLWT) {
            connected = mqttClient->connect(storage->id, storage->mqttUser, storage->mqttPassword, storage->mqttLWTTopic, 1, 1, storage->mqttOfflineMessage);
        }
        else {
            connected = mqttClient->connect(storage->id, storage->mqttUser, storage->mqttPassword);
        }
        if (connected) {
            Serial.println("MQTT Connected.");
            mqttClient->publish(storage->mqttLWTTopic, storage->mqttOnlineMessage, 1);
            mqttClient->subscribe((String(storage->mqttTopic) + "/#").c_str());
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient->state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}*/
