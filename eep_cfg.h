/***********************************************************************************************************************
   @file   wmc_app.cpp
   @brief  Main application of WifiManualControl (WMC).
 **********************************************************************************************************************/

#ifndef EEP_CFG_H
#define EEP_CFG_H

/***********************************************************************************************************************
   I N C L U D E S
 **********************************************************************************************************************/

/***********************************************************************************************************************
   C L A S S E S
 **********************************************************************************************************************/

class EepCfg
{
public:
    static const uint8_t EepromVersion = 4; /* Version of data in EEPROM. */

    static const int EepromVersionAddress         = 1;   /* EEPROM address version info. */
    static const int AcTypeControlAddress         = 2;   /* EEPROM address for "AC" type control */
    static const int StaticIpAddress              = 4;   /* EEPROM address static or dynamic IP address */
    static const int SsidAddress                  = 10;  /* EEPROM Address of Ssid name */
    static const int SsidPasswordAddress          = 55;  /* EEPROM Address of Ssid password */
    static const int EepIpAddressZ21              = 112; /* EEPROM Address of IP address of Z21. */
    static const int EepIpAddressWmc              = 116; /* EEPROM Address of IP address of Wmc. */
    static const int EepIpSubnet                  = 120; /* EEPROM Address of IP subnet of Wmc. */
    static const int EepIpGateway                 = 124; /* EEPROM Address of gateway ip. */
    static const int locLibEepromAddressData      = 140; /* Start in EEPROM address loc data. */
    static const int locLibEepromAddressNumOfLocs = 132; /* EEPROM address num of locs. */
};
#endif
