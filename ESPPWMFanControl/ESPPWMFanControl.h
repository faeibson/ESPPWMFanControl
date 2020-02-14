/*
 * 
 * ESP PWM Fan Control 1.0-beta2 - © 2020 Fabian Brain. https://github.com/faeibson 
 * Thanks to all you guys contributing all these awesome libraries to the community!
 * 
 * This project is licensed unter the GNU General Public License v3.0
 */

#ifndef _ESP_PWM_FAN_CONTROL_H_
#define _ESP_PWM_FAN_CONTROL_H_

#include <brzo_i2c.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
//#include <PubSubClient.h>
#include <SSD1306Brzo.h>
#include <WiFiManager.h>
#include <limits>


/* CLASSES */
#include "FanControlHelper.h"
#include "Storage.h"


/* PROGMEM ASSETS */
#include "progmem_assets.h"


/* SETTINGS - CHANGE AS YOU NEED */
#include "ESPPWMFanControlSettings.h"


#endif
