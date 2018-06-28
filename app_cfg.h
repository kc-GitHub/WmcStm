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

/**
 * Pin definitions for the TFT display.
 */
#define APP_CFG_RST D2
#define APP_CFG_DC D1
#define APP_CFG_CS D8

#if APP_CFG_PCB_VERSION == APP_CFG_PCB_VERSION_REV01
#define APP_CFG_SCL D4
#define APP_CFG_SDA D3
#else
#define APP_CFG_SCL D3
#define APP_CFG_SDA D4
#endif
#endif
