
#ifndef _FAN_CONTROL_HELPER_H_
#define _FAN_CONTROL_HELPER_H_

#include <Arduino.h>

struct FanControlHelper {
    static float calculateNTCTemperature(uint16_t measuredAnalogValue, uint16_t ntcValueRange, float ntcVoltage, uint32_t ntcReferenceResistance, float ntcReferenceTemperature, uint16_t ntcBeta, uint32_t ntcPullUpResistorValue, uint8_t ntcRoundTemperatureToDecimals);
    static void convertByteToChar(byte *bytes, char *chars, unsigned int count);
    static uint8_t convertCharToHex(char c);
    static uint8_t convertHexStringToInt(char *s);
    static String urlEncode(String url);
};

#endif
