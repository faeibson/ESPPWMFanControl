
#ifndef _ESP_PWM_FAN_CONTROL_SETTINGS_H_
#define _ESP_PWM_FAN_CONTROL_SETTINGS_H_


/* SETTINGS CONSTANTS - CHANGE AS YOU NEED */

namespace settings {
    
    const bool debug = false;                               // enable additional debug messages
    const bool simulation_mode = false;                     // simulate the NTC temperature measurements instead of actually reading the analog pin (i.e. random values)
    
    // NTC settings
    const uint8_t ntcAnalogPin = A0;                        // the analog pin the NTC is connected to
    const uint16_t ntcValueRange = 1023;                    // the NTC's value range (usually equals the analog pin's maximum value)
    const uint16_t ntcBeta = 3950;                          // the NTC's beta value in Kelvin
    const float ntcReferenceTemperature = 298.15;           // the NTC's reference temperature in Kelvin (usually 298.15 K = 25 deg. C = 77 deg. F
    const uint32_t ntcReferenceResistance = 10000;          // NTC resistance at ntcReferenceTemperature in Ohm
    const float ntcVoltage = 3.3;                           // the voltage the NTC is connected to in Volt
    const uint32_t ntcPullUpResistorValue = 150000;         // pull up resistor between NTC and V+ in Ohm
    const uint8_t ntcRoundTemperatureToDecimals = 2;        // round temperature to n decimals

    const uint8_t sclPin = 5;                               // SCL pin for I2C
    const uint8_t sdaPin = 4;                               // SDA pin for I2C
    
    // at the moment, only 3 fan channels and a maximum of 9 control sets are supported
    const uint8_t fanCount = 3;
    const uint8_t fanPins[] = { 13, 12, 0 };
    const uint8_t maxFanControlSets = 9;

    const uint16_t pwmFrequency = 25000;                    // necessary PWM frequency is 25 kHz
    
    // only increase this, if you know what analogWriteRange your hardware is capable of at 21~28 kHz PWM frequency and you want finer steps
    // smaller range enables higher PWM frequencies
    const uint16_t pwmValueRange = 4;                       // PWM value range - 4 equals 5 steps: 0, 25, 50, 75, 100 percent

    // file names in SPIFFS - not PROGMEM since SPIFFS library can't handle it
    /*const char cssFileName[] = "/style.css";
    const char faviconFileName[] = "/favicon.ico";
    const char javaScriptFileName[] = "/script.js";*/
    const char settingsJsonFileName[] = "/settings.json";
}

#endif
