
#include "Storage.h"

Storage::Storage(const char *t_storageFileName, const uint8_t *t_fanPins, const uint8_t t_fanCount, const uint16_t t_analogWriteRange, const uint8_t t_maxFanControlSets) :
        debug(false),
        fanCount(t_fanCount),
        fanPins(t_fanPins),
        fanPercentStepSize(100/t_analogWriteRange),
        maxFanControlSets(t_maxFanControlSets),
        storageFileName(t_storageFileName)
{
    init();
}


Storage::Storage(const char *t_storageFileName, const uint8_t *t_fanPins, const uint8_t t_fanCount, const uint16_t t_analogWriteRange, const uint8_t t_maxFanControlSets, bool t_debug) :
        debug(t_debug),
        fanCount(t_fanCount),
        fanPins(t_fanPins),
        fanPercentStepSize(100/t_analogWriteRange),
        maxFanControlSets(t_maxFanControlSets),
        storageFileName(t_storageFileName)
{
    init();
}

  
Storage::~Storage() {
    SPIFFS.end();
    
    for(uint8_t i = 0; i < fanControlSetCount; i++) {
        delete fanControlSets[i];
    }
    //delete [] fanControlSets;
    delete [] fanSpeeds;
    delete [] fanSpeedsActive;
}


void Storage::init() {
    SPIFFS.begin();

    fanSpeeds = new uint8_t[fanCount];
    fanSpeedsActive = new uint8_t[fanCount];
    
    for(uint8_t i = 0; i < fanCount; i++) {
        fanSpeeds[i] = 100;
        fanSpeedsActive[i] = 100;
    }
}


String Storage::getTemporaryMessage() {
    String buffer = temporaryMessage;
    temporaryMessage.clear();
    return buffer;
}


String Storage::json() {
    DynamicJsonDocument doc(jsonBufferSize + 400); // 400 for runtime vars

    doc["activeFanControlSet"] = activeFanControlSet;
    doc["automaticFanControlEnabled"] = automaticFanControlEnabled;
    doc["displayAddress"] = displayAddress;
    doc["displayDurationPerMinute"] = displayDurationPerMinute;
    doc["displayEnabled"] = displayEnabled;
    doc["displayFlipScreen"] = displayFlipScreen;
    doc["displayIsOn"] = displayIsOn;
    doc["id"] = id;
    
    doc["ntcTemperature"] = ntcTemperature;
    unsigned long ntcTemperatureTimeSinceMeasurement = millis() - ntcTemperatureTime;
    doc["ntcTemperatureTimeSinceMeasurement"] = ntcTemperatureTimeSinceMeasurement;
    doc["pageRefreshTime"] = pageRefreshTime;
    
    doc["fanCount"] = fanCount;
    doc["fanPercentStepSize"] = fanPercentStepSize;
    doc["maxFanControlSets"] = maxFanControlSets;

    JsonArray jsonFanPins = doc.createNestedArray("fanPins");
    copyArray(fanPins, fanCount, jsonFanPins);
    JsonArray jsonFanSpeeds = doc.createNestedArray("fanSpeeds");
    copyArray(fanSpeeds, fanCount, jsonFanSpeeds);
    JsonArray jsonFanSpeedsActive = doc.createNestedArray("fanSpeedsActive");
    copyArray(fanSpeedsActive, fanCount, jsonFanSpeedsActive);
    
    /*doc["fanSpeed1"] = fanSpeed1;
    doc["fanSpeed2"] = fanSpeed2;
    doc["fanSpeed3"] = fanSpeed3;
    doc["fanSpeedActive1"] = fanSpeedActive1;
    doc["fanSpeedActive2"] = fanSpeedActive2;
    doc["fanSpeedActive3"] = fanSpeedActive3;*/

    jsonFanControlSets(&doc);
  
    String output;
    serializeJson(doc, output);
    return output;
}


JsonArray Storage::jsonFanControlSets(DynamicJsonDocument *doc) {
    JsonArray jsonFanControlSets = doc->createNestedArray("fanControlSets");
    
    for(uint8_t i = 0; i < fanControlSetCount; i++) {
        JsonObject jsonFanControlSet = jsonFanControlSets.createNestedObject();
        JsonArray jsonFanControlSetFanSpeeds = jsonFanControlSet.createNestedArray("fanSpeeds");

        /*for(uint8_t j = 0; j < (sizeof(fanControlSets[0].fanSpeeds) / sizeof(uint8_t)); j++) {
            jsonFanControlSetFanSpeeds.add(fanControlSets[i].fanSpeeds[j]);
        }*/
        copyArray(fanControlSets[i]->fanSpeeds, fanControlSets[i]->fanCount, jsonFanControlSetFanSpeeds);
        
        jsonFanControlSet["tempThreshold"] = fanControlSets[i]->tempThreshold;
    }

    return jsonFanControlSets;
}
        
void Storage::load() {
    File f = SPIFFS.open(storageFileName, "r");
    if (!f) {
        Serial.println(F("Error reading settings from file. Leaving defaults.\n"));
    }
    else {
        DynamicJsonDocument doc(jsonBufferSize);
        DeserializationError error = deserializeJson(doc, f);
        if (error) {
            Serial.print(F("Error deserializing settings. Leaving defaults. deserializeJson() error: "));
            Serial.print(error.c_str());
            Serial.println(F("\n"));
        }
        else {
            doc.shrinkToFit();
            strlcpy(id,
                    doc["id"] | "ESPPWMFanControl",
                    sizeof(id));
        
            /*fanSpeed1 = doc["fanSpeed1"] | 100;
            fanSpeed2 = doc["fanSpeed2"] | 100;
            fanSpeed3 = doc["fanSpeed3"] | 100;
            fanSpeedActive1 = doc["fanSpeedActive1"] | 100;
            fanSpeedActive2 = doc["fanSpeedActive2"] | 100;
            fanSpeedActive3 = doc["fanSpeedActive3"] | 100;*/
            
            pageRefreshTime = doc["pageRefreshTime"] | 10;
            
            displayEnabled = doc["displayEnabled"] | true;
            displayAddress = doc["displayAddress"] | 0x3C;
            displayDurationPerMinute = doc["displayDurationPerMinute"] | 30;
            displayFlipScreen = doc["displayFlipScreen"] | false;
            
            automaticFanControlEnabled = doc["automaticFanControlEnabled"] | false;
            /*fanControlFan1_1 = doc["fanControlFan1_1"] | 100;
            fanControlFan1_2 = doc["fanControlFan1_2"] | 100;
            fanControlFan1_3 = doc["fanControlFan1_3"] | 100;
            fanControlFan1_4 = doc["fanControlFan1_4"] | 100;
            fanControlFan2_1 = doc["fanControlFan1_1"] | 100;
            fanControlFan2_2 = doc["fanControlFan1_2"] | 100;
            fanControlFan2_3 = doc["fanControlFan1_3"] | 100;
            fanControlFan2_4 = doc["fanControlFan1_4"] | 100;
            fanControlFan3_1 = doc["fanControlFan1_1"] | 100;
            fanControlFan3_2 = doc["fanControlFan1_2"] | 100;
            fanControlFan3_3 = doc["fanControlFan1_3"] | 100;
            fanControlFan3_4 = doc["fanControlFan1_4"] | 100;
            fanControlTemp_1 = doc["fanControlTemp_1"] | 25;
            fanControlTemp_2 = doc["fanControlTemp_2"] | 25;
            fanControlTemp_3 = doc["fanControlTemp_3"] | 25;
            fanControlTemp_4 = doc["fanControlTemp_4"] | 25;*/

            // may be configurable in upcoming versions
            /*fanCount = doc["fanCount"] || 3;
            JsonArray jsonFanPins = doc["fanPins"].as<JsonArray>();
            copyArray(jsonFanPins, fanPins);*/
            
            JsonArray jsonFanSpeeds = doc.createNestedArray("fanSpeeds");
            copyArray(fanSpeeds, fanCount, jsonFanSpeeds);
            JsonArray jsonFanSpeedsActive = doc.createNestedArray("fanSpeedsActive");
            copyArray(fanSpeedsActive, fanCount, jsonFanSpeedsActive);

            delete [] fanControlSets;
            JsonArray jsonFanControlSets = doc["fanControlSets"].as<JsonArray>();
            
            fanControlSetCount = jsonFanControlSets.size();
            fanControlSets = new FanControlSet*[fanControlSetCount];
            
            for(uint8_t i = 0; i < fanControlSetCount; i++) {
                JsonObject jsonFanControlSet = jsonFanControlSets[i].as<JsonObject>();
                JsonArray jsonFanControlSetFanSpeeds = jsonFanControlSet["fanSpeeds"].as<JsonArray>();
                
                /*for(uint8_t j = 0; j < min(jsonFanControlSetFanSpeeds.size(), sizeof(fanControlSets[i].fanSpeeds)); j++) {
                    fanControlSets[i].fanSpeeds[j] = jsonFanControlSetFanSpeeds[j];
                }*/

                fanControlSets[i] = new FanControlSet(fanCount);
                copyArray(jsonFanControlSetFanSpeeds, fanControlSets[i]->fanSpeeds, fanCount);
                
                fanControlSets[i]->tempThreshold = jsonFanControlSet["tempThreshold"];
            }

            if(fanControlSetCount > 1) {
                sortFanControlSets();
            }
        
            Serial.println(F("Settings read from SPIFFS.\n"));
        }
    }

    if(debug) {
        print();
    }
    f.close();
}


uint8_t Storage::normalizeFanSpeedValue(uint8_t fanSpeed) { // normalize fan speeds from 0 to 100 and adjust to pwm value range
    return std::min(std::max(0, (fanSpeed / fanPercentStepSize) * fanPercentStepSize), 100);
}


void Storage::print() {
    Serial.println(F("\nDebugging settings:\n> Permanent settings:"));
    Serial.print(F(">> automaticFanControlEnabled: ")); Serial.println(automaticFanControlEnabled);
    Serial.print(F(">> displayAddress: ")); Serial.println(displayAddress, HEX);
    Serial.print(F(">> displayDurationPerMinute: ")); Serial.println(displayDurationPerMinute);
    Serial.print(F(">> displayEnabled: ")); Serial.println(displayEnabled);
    Serial.print(F(">> displayFlipScreen: ")); Serial.println(displayFlipScreen);
    Serial.print(F(">> fanCount: ")); Serial.println(fanCount);
    for(uint8_t i = 0; i < fanCount; i++) {
        Serial.print(F(">> fanPins[")); Serial.print(i); Serial.print("]: "); Serial.println(fanPins[i]);
    }
    for(uint8_t i = 0; i < fanCount; i++) {
        Serial.print(F(">> fanSpeeds[")); Serial.print(i); Serial.print("]: "); Serial.println(fanSpeeds[i]);
    }
    for(uint8_t i = 0; i < fanCount; i++) {
        Serial.print(F(">> fanSpeedsActive[")); Serial.print(i); Serial.print("]: "); Serial.println(fanSpeeds[i]);
    }
    Serial.print(F(">> fanControlSetCount: ")); Serial.println(fanControlSetCount);
    Serial.print(F(">> id: ")); Serial.println(id);
    Serial.print(F(">> pageRefreshTime: ")); Serial.println(pageRefreshTime);
    Serial.println(F("\n> Runtime variables:"));
    Serial.print(F(">> activeFanControlSet: ")); Serial.println(activeFanControlSet);
    Serial.print(F(">> displayIsOn: ")); Serial.println(displayIsOn);
    Serial.print(F(">> ntcTemperature: ")); Serial.println(ntcTemperature);
    Serial.println();
}


void Storage::save() {
    File f = SPIFFS.open(storageFileName, "w");
    
    if (f) {
        DynamicJsonDocument doc(jsonBufferSize);
        doc["automaticFanControlEnabled"] = automaticFanControlEnabled;
        doc["displayAddress"] = displayAddress;
        doc["displayDurationPerMinute"] = displayDurationPerMinute;
        doc["displayEnabled"] = displayEnabled;
        doc["displayFlipScreen"] = displayFlipScreen;
        doc["id"] = id;
        doc["pageRefreshTime"] = pageRefreshTime;
        
        /*JsonArray jsonFanPins = doc.createNestedArray("fanPins");
        copyArray(fanPins, jsonFanPins);*/
        JsonArray jsonFanSpeeds = doc.createNestedArray("fanSpeeds");
        copyArray(fanSpeeds, fanCount, jsonFanSpeeds);
        JsonArray jsonFanSpeedsActive = doc.createNestedArray("fanSpeedsActive");
        copyArray(fanSpeedsActive, fanCount, jsonFanSpeedsActive);
        
        /*doc["fanSpeed1"] = fanSpeed1;
        doc["fanSpeed2"] = fanSpeed2;
        doc["fanSpeed3"] = fanSpeed3;
        doc["fanSpeedActive1"] = fanSpeedActive1;
        doc["fanSpeedActive2"] = fanSpeedActive2;
        doc["fanSpeedActive3"] = fanSpeedActive3;*/

        if(fanControlSetCount > 1) {
            sortFanControlSets();
        }
        jsonFanControlSets(&doc);
    
        size_t count = serializeJson(doc, f);
        if (count == 0) {
            Serial.println(F("Error saving settings to SPIFFS!\n"));
        }
        else {
            Serial.print(F("Settings written successfully. Bytes written: ")); Serial.println(count);
            if(debug) {
                print();
            }
        }
        f.close();
    }
    else {
        Serial.println(F("Error opening/creating settings file.\n"));
    }
}

void Storage::setTemporaryMessage(String message) {
    temporaryMessage = message;
}

void Storage::sortFanControlSets() {
    if(fanControlSetCount > 2) {
        // that does not work anymore, fanControlSets is now an array of pointers
        //std::sort(fanControlSets, fanControlSets + fanControlSetCount, FanControlSet::compareAscending);
        
        bool swapped = true;
        
        while(swapped) {
            swapped = false;
            
            // do not move the default fan control set - row 0
            for(uint8_t i = 1; i < fanControlSetCount - 1; i++) {
                if(FanControlSet::compareDescending(*fanControlSets[i], *fanControlSets[i + 1])) {
                    FanControlSet *temp = fanControlSets[i];
                    fanControlSets[i] = fanControlSets[i + 1];
                    fanControlSets[i + 1] = temp;
                    swapped = true;
                }
            }
        }
    }
}
