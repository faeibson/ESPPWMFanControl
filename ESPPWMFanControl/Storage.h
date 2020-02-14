
#ifndef _STORAGE_H_
#define _STORAGE_H_

#include <algorithm>
#include <FS.h>
#include <ArduinoJson.h>
#include "FanControlSet.h"

struct Storage {
    // constants
    const uint8_t fanCount = 0;
    const uint8_t *fanPins;
    const uint8_t fanPercentStepSize = 25;   // calculated PWM percentage steps
    const uint8_t maxFanControlSets = 9;

    // runtime variables
    uint8_t activeFanControlSet = 0;
    bool displayIsOn = false;
    unsigned long displayLastOnTime = 0;
    float ntcTemperature = 0;
    unsigned long ntcTemperatureTime = 0;
    bool resetESP = false;
    unsigned long resetESPTime = 0;

    // permanent settings
    bool automaticFanControlEnabled = false;
    
    unsigned char displayAddress = 0x3C;
    uint8_t displayDurationPerMinute = 60;
    bool displayEnabled = false;
    bool displayFlipScreen = false;
    
    char *id = "ESPPWMFanControl";

    uint8_t *fanSpeeds;
    uint8_t fanSpeed1 = 100;
    uint8_t fanSpeed2 = 100;
    uint8_t fanSpeed3 = 100;
    uint8_t *fanSpeedsActive;
    uint8_t fanSpeedActive1 = 100;
    uint8_t fanSpeedActive2 = 100;
    uint8_t fanSpeedActive3 = 100;
    
    /*uint8_t fanControlFan1_1;
    uint8_t fanControlFan1_2;
    uint8_t fanControlFan1_3;
    uint8_t fanControlFan1_4;
    uint8_t fanControlFan2_1;
    uint8_t fanControlFan2_2;
    uint8_t fanControlFan2_3;
    uint8_t fanControlFan2_4;
    uint8_t fanControlFan3_1;
    uint8_t fanControlFan3_2;
    uint8_t fanControlFan3_3;
    uint8_t fanControlFan3_4;
    float fanControlTemp_1;
    float fanControlTemp_2;
    float fanControlTemp_3;
    float fanControlTemp_4;*/
    uint8_t fanControlSetCount = 0;
    //FanControlSet defaultFanControlSet; // TODO move first fan control set (without a threshold) to its own property...
    FanControlSet **fanControlSets;
    
    uint8_t pageRefreshTime = 0;

    // public methods
    Storage(const char *t_storageFileName, const uint8_t *t_fanPins, const uint8_t t_fanCount, const uint16_t t_analogWriteRange, const uint8_t t_maxFanControlSets);
    Storage(const char *t_storageFileName, const uint8_t *t_fanPins, const uint8_t t_fanCount, const uint16_t t_analogWriteRange, const uint8_t t_maxFanControlSets, bool t_debug);
    ~Storage();
    String getTemporaryMessage();
    String json();
    JsonArray jsonFanControlSets(DynamicJsonDocument *doc);
    void load();
    uint8_t normalizeFanSpeedValue(uint8_t fanSpeed);
    void print();
    void save();
    void setTemporaryMessage(String message);
    void sortFanControlSets();

    private:
        size_t jsonBufferSize = 2000;
        bool debug;
        const char *storageFileName;
        String temporaryMessage;

        void init();
};

#endif
