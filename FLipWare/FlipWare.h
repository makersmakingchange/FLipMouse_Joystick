
/*
     FLipWare - AsTeRICS Foundation
     For more info please visit: https://www.asterics-foundation.org

     Module: FlipWare.h  - main header file

        This firmware allows control of HID functions via FLipmouse module and/or AT-commands
        For a description of the supported commands see: commands.h

   For a list of supported AT commands, see commands.h / commands.cpp

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; See the GNU General Public License:
   http://www.gnu.org/licenses/gpl-3.0.en.html

*/


#ifndef _FLIPWARE_H_
#define _FLIPWARE_H_

#include <Arduino.h>
#include <Wire.h>
#include <Mouse.h>
#include <Keyboard.h>
#include <Joystick.h>
#include <EEPROM.h>
#include <string.h>
#include <stdint.h>
#include "commands.h"
#include "eeprom.h"
#include "buttons.h"
#include "infrared.h"
#include "bluetooth.h"
#include "hid_hal.h"

#define VERSION_STRING "v3.6.2"

//  V3.6.2:  added sensor information to AT ID reply, updated sensorboard profiles for piezoresistive SMD sensor board
//  V3.6.1:  integrated support for DPS310 pressure sensor (new sip/puff daughter-board)
//  V3.5:  reduced USB HID report frequency (fixes lost keyboard reports)
//  V3.4:  improved MPRLS pressure sensor processing
//  V3.3.1:  fixed IR-command name bug
//  V3.3:  added Bluetooth Joystick
//  V3.2:  changed pinning to PCB v3.2
//  V3.00: changed platform to Arduino Nano RP2040 Connect
//  V2.12.1: fixed keystring buffer problem
//  V2.12: improved modularisation and source code documentation, added LC-display support and elliptical deadzone
//  V2.11: eeprom access optimization and support for deletion / update of individual slots
//  V2.10: code size reduction: using floating point math, removed debug level control via AT E0, AT E1 and AT E2
//          added macro command description to the user manual
//  V2.9:  implemented drift correction for small deadzones, removed gain up/down/left/right,
//          added AT commands for drift correction, modified calculation of acceleration
//  V2.8.3: switched to semantic version numbering, increased acceleration factors
//  V2.8.2: corrected memory bugs (index / heap overflows), added slot copy feature (in GUI)
//  V2.8.1: corrected bug in deadzone calculation for keyboard actions, improved stable time for strong sip/puff functions
//  V2.8: improved cursor control by using polar coordinates and damping
//  V2.7: improved IR command recording and playback (IR hold repeats codes, optionally append off-sequence)
//  V2.6: updated API for KEY commands (added KT, changed KP) and Mouse Click commands (added toggle clicks)
//  V2.5: added stick rotation options, improved acoustic slot feedback, improved keycode handling,
//        removed Teensy2.0++ support, new AT commands: clear IR memory, route HID to BT/USB/both
//  V2.4: added support for acceleration, maximum speed and command macros
//	V2.3: added support for internal Bluetooth Addon
//  V2.2: added new EEPROM handling and IR-Command support
//  V2.0: extended AT command set, TeensyLC support, external EEPROM
//  V1.0: extended AT command set, GUI compatibility


// Optional Debug Output Control

//#define DEBUG_OUTPUT_FULL      // if full debug output is desired
//#define DEBUG_OUTPUT_MEMORY    // enables eeprom.cpp debugging, showing memory access
//#define DEBUG_OUTPUT_KEYS      // enable keys.cpp debugging, showing key press/release events and keycode lookup
//#define DEBUG_OUTPUT_IR      	 // enable infrared.cpp debugging, showing whats happening on IR recv/send
//#define DEBUG_OUTPUT_SENSORS 	 // enable sensors.cpp debugging, showing whats happening on sensor reading & init
//#define DEBUG_DELAY_STARTUP 	 // enable a 3s delay after Serial.begin and before all the other stuff.
//#define DEBUG_NO_TONE          // disable tones, to avoid annoying other passengers when programming on the train :-)
//#define DEBUG_PRESSURE_RAWVALUES // raw output of pressure values and filtered output
//#define DEBUG_MPRLS_ERRORFLAGS // continously print error flags of MPRLS

#define BUILD_FOR_RP2040        // enable a build for RP2040. There are differences in eeprom & infrared handling.

/**
   global constant definitions
*/
#define UPDATE_INTERVAL     8    // update interval for performing HID actions (in milliseconds)
#define DEFAULT_CLICK_TIME  8    // time for mouse click (loop iterations from press to release)
#define CALIBRATION_PERIOD  1000  // approx. 1000ms calibration time

// RAM buffers and memory constraints
#define WORKINGMEM_SIZE         300    // reserved RAM for working memory (command parser, IR-rec/play)
#define MAX_KEYSTRING_LEN (WORKINGMEM_SIZE-3)   // maximum length for AT command parameters
#define MAX_NAME_LEN  15               // maximum length for a slotname or ir name
#define MAX_KEYSTRINGBUFFER_LEN 500    // maximum length for all string parameters of one slot

// direction identifiers
#define DIR_E   1   // east
#define DIR_NE  2   // north-east
#define DIR_N   3   // north
#define DIR_NW  4   // north-west
#define DIR_W   5   // west
#define DIR_SW  6   // sout-west
#define DIR_S   7   // south
#define DIR_SE  8   // south-east

/**
   SlotSettings struct
   contains parameters for current slot
*/
struct SlotSettings {

  char slotName[MAX_NAME_LEN];   // slotname (@warning: must be always the first element, storing relies on that!)
  uint16_t keystringBufferLen;   
  
  uint8_t  stickMode;  // alternative(0), mouse(1), joystick (2,3,4)
  uint8_t  ax;     // acceleration x
  uint8_t  ay;     // acceleration y
  int16_t  dx;     // deadzone x
  int16_t  dy;     // deadzone y
  uint16_t ms;     // maximum speed
  uint16_t ac;     // acceleration time
  uint16_t ts;     // threshold sip
  uint16_t tp;     // threshold puff
  uint8_t  ws;     // wheel stepsize
  uint16_t sp;     // threshold strong puff
  uint16_t ss;     // threshold strong sip
  uint8_t  gv;     // gain vertical drift compensation
  uint8_t  rv;     // range vertical drift compensation
  uint8_t  gh;     // gain horizontal drift compensation
  uint8_t  rh;     // range horizontal drift compensation
  uint16_t ro;     // orientation (0,90,180,270)
  uint8_t  bt;     // bt-mode (0,1,2)
  uint8_t  sb;     // sensorboard-profileID (0,1,2,3)
  uint32_t sc;     // slotcolor (0x: rrggbb)
  char kbdLayout[6];
};

/**
   SensorData structs
   contain working data of sensors (raw and processed values)
*/
struct SensorData {
  int x, y, xRaw,yRaw;
  int pressure;
  float deadZone, force, forceRaw, angle;
  uint8_t dir;
  int8_t autoMoveX,autoMoveY;
  int xDriftComp, yDriftComp;
  int xLocalMax, yLocalMax;  
};

struct I2CSensorValues {
  int xRaw,yRaw;
  int pressure;
  uint16_t calib_now;
  mutex_t sensorDataMutex; // for synchronization of data access between cores
};

/**
   extern declarations of functions and data structures 
   which can be accessed from different modules
*/
extern char moduleName[];
extern uint8_t actSlot;
extern uint8_t addonUpgrade;
extern struct SensorData sensorData;
extern struct I2CSensorValues sensorValues;
extern struct SlotSettings slotSettings; 
extern const struct SlotSettings defaultSlotSettings;
extern uint8_t workingmem[WORKINGMEM_SIZE];            // working memory  (command parser, IR-rec/play)
extern char keystringBuffer[MAX_KEYSTRINGBUFFER_LEN];  // storage for all button string parameters of a slot

/**
   set the correct strcpy/strcmp functions for TeensyLC / ARM)
*/
#define strcpy_FM   strcpy
#define strcmp_FM   strcmp
typedef char* uint_farptr_t_FM;

/**
 * Some define checks to warn from building in debug settings
 */
#ifdef DEBUG_OUTPUT_MEMORY
  #warning "DEBUG_OUTPUT_MEMORY active! (GUI might not work)"
#endif
#ifdef DEBUG_OUTPUT_FULL
  #warning "DEBUG_OUTPUT_FULL active! (GUI might not work)"
#endif
#ifdef DEBUG_OUTPUT_KEYS
  #warning "DEBUG_OUTPUT_KEYS active! (GUI might not work)"
#endif
#ifdef DEBUG_OUTPUT_IR
  #warning "DEBUG_OUTPUT_IR active! (GUI might not work)"
#endif
#ifdef DEBUG_OUTPUT_SENSORS
  #warning "DEBUG_OUTPUT_SENSORS active! (GUI might not work)"
#endif
#ifdef DEBUG_DELAY_STARTUP
  #warning "DELAY_STARTUP is active, do not release this way!"
#endif
#ifdef DEBUG_NO_TONE
  #warning "DEBUG_NO_TONE is defined, do not release this way!"
#endif
#ifdef DEBUG_PRESSURE_RAWVALUES
  #warning "DEBUG_PRESSURE_RAWVALUES is defined, do not release this way!"
#endif
#ifdef DEBUG_MPRLS_ERRORFLAGS
  #warning "DEBUG_MPRLS_ERRORFLAGS is defined, do not release this way!"
#endif

#endif
