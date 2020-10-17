/***********************************************************************************************************************
   @file   app_cfg.h
   @brief  Settings for WMC/XMC tft module.
 **********************************************************************************************************************/

#ifndef APP_CFG_H
#define APP_CFG_H

/***********************************************************************************************************************
   I N C L U D E S
 **********************************************************************************************************************/
#include <Arduino.h>

/***********************************************************************************************************************
   C L A S S E S
 **********************************************************************************************************************/

#define DEVICE_NAME_PREFIX	"WMC-2_"

/**
 * Definitions for pcb version.
 */
#define APP_CFG_PCB_VERSION_DEFAULT 0
#define APP_CFG_PCB_VERSION_REV01 1

/**
 * On the PCB the SLCK and MOSI are mixed up...
 */
#define APP_CFG_PCB_VERSION APP_CFG_PCB_VERSION_REV01

/**
 * Definitions for the possible micro controller.
 */
#define APP_CFG_UC_ESP8266 0
#define APP_CFG_UC_STM32 1
#define APP_CFG_UC_ATMEL 2

/**
 * Definition for the micro used in this application.
 */
#define APP_CFG_UC APP_CFG_UC_ESP8266

// Pin definition for TFT was don in User_Setup.h in TFT_eSPI library

#define ENABLE_SERIAL_DEBUG     0 // Set to 1 to enable serial debug

#define TFT_WIDTH               240
#define TFT_HEIGHT              240
#define FUNCTION_BUTTON_SIZE    25

#define PIN_KEYBOARD_C0         2 // D4 on WEMOS D1 Mini
#define PIN_KEYBOARD_C1         0 // D3 on WEMOS D1 Mini
#define PIN_KEYBOARD_C2         3 // RX on WEMOS D1 Mini
#define PIN_KEYBOARD_C3         1 // TX on WEMOS D1 Mini
#define KEYBOARD_SCAN_TIME      60000 //12000 us

#define KEYCODE_ENCODER_BTN     132 // the encoder push button is connected to the keypad to save pins
#define KEYCODE_POWER           68
#define KEYCODE_MENU            36
#define KEYCODE_MODE            20
#define KEYCODE_LEFT            9
#define KEYCODE_RIGHT           35
#define KEYCODE_0               4
#define KEYCODE_1               10
#define KEYCODE_2               2
#define KEYCODE_3               19
#define KEYCODE_4               6
#define KEYCODE_5               3
#define KEYCODE_6               11
#define KEYCODE_7               34
#define KEYCODE_8               17
#define KEYCODE_9               8

#define MAX_FUNCTION_BUTTONS    10  // Maximum count of usable function buttons inclusive light (0)

#endif
