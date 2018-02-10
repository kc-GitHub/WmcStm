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
    static const uint8_t EepromVersion            = 3;   /* Version of data in EEPROM. */
    static const int EepromVersionAddress         = 1;   /* EEPROM address version info. */
    static const int AcTypeControlAddress         = 2;   /* EEPROM address for "AC" type control */
    static const int SsidAddress                  = 10;  /* Address of Ssid name */
    static const int SsidPasswordAddress          = 55;  /* Address of Ssid password */
    static const int EepIpAddress                 = 112; /* Aadress of IP address in EEPROM. */
    static const int locLibEepromAddressData      = 120; /* Start in EEPROM address loc data. */
    static const uint8_t locLibMaxNumberOfLocs    = 64;  /* Max number of locs in EEPROM. */
    static const int locLibEepromAddressNumOfLocs = 119; /* EEPROM address num of locs. */
};
#endif
