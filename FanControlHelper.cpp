
#include "FanControlHelper.h"

float FanControlHelper::calculateNTCTemperature(uint16_t measuredAnalogValue, uint16_t ntcValueRange, float ntcVoltage, uint32_t ntcReferenceResistance, float ntcReferenceTemperature, uint16_t ntcBeta, uint32_t ntcPullUpResistorValue, uint8_t ntcRoundTemperatureToDecimals) {
    float Vout, Rt, measuredTemperature_kelvin, measuredTemperature_celsius;

    // analog value to analog pin voltage
    Vout = measuredAnalogValue * ntcVoltage / ntcValueRange;

     // resistance of the NTC
    Rt = ntcPullUpResistorValue * Vout / (ntcVoltage - Vout);
    
    // NTC resistance to temperature
    measuredTemperature_kelvin = 1 / (1 / ntcReferenceTemperature + log(Rt / ntcReferenceResistance) / ntcBeta);

    // Kelvin to deg. Celsius
    measuredTemperature_celsius = measuredTemperature_kelvin - 273.15;

    // rounding factor
    uint32_t ntcRoundTemperatureToDecimalsSquared = std::pow(10, ntcRoundTemperatureToDecimals);

    // round measured temperature
    measuredTemperature_celsius = round(measuredTemperature_celsius * ntcRoundTemperatureToDecimalsSquared) / ntcRoundTemperatureToDecimalsSquared;
  
    return measuredTemperature_celsius;
}

void FanControlHelper::convertByteToChar(byte *bytes, char *chars, unsigned int count) {
    for (unsigned int i = 0; i < count; i++) {
        chars[i] = (char)bytes[i];
    }
    chars[count] = (char)0;
}

uint8_t FanControlHelper::convertCharToHex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

uint8_t FanControlHelper::convertHexStringToInt(char *s) {
    uint8_t x = 0;
    if(sizeof(s) / sizeof(char) > 2) {  // skip "0x"
        if(s[0] == '0' && s[1] == 'x') {
            s += 2;
        }
    }
    for(;;) {
        char c = *s;
        if (c >= '0' && c <= '9') {
            x *= 16;
            x += c - '0';
        }
        else if (c >= 'A' && c <= 'F') {
            x *= 16;
            x += (c - 'A') + 10;
        }
        else if (c >= 'a' && c <= 'f') {
            x *= 16;
            x += (c - 'a') + 10;
        }
        else break;
        s++;
    }
    return x;
}

String FanControlHelper::urlEncode(String url) {
    String encodedString = "";
    char c;
    char code0;
    char code1;
    
    for(int i = 0; i < url.length(); i++) {
        c = url.charAt(i);
      
        if(c == ' ') {
            encodedString += '+';
        } else if(isalnum(c)) {
            encodedString += c;
        } else {
            code1 = (c & 0xf) + '0';
            if((c & 0xf) > 9){
                code1 = (c & 0xf) - 10 + 'A';
            }
            c = (c >> 4) & 0xf;
            code0 = c+'0';
            if(c > 9){
                code0 = c - 10 + 'A';
            }
            encodedString += '%';
            encodedString += code0;
            encodedString += code1;
        }
        yield();
    }
    return encodedString;
}
