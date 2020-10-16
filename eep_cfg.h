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
    static const uint8_t EepromVersion = 6; /* Version of data in EEPROM. */

    static const int EepromVersionAddress         = 1;   /* EEPROM address version info. */
    static const int AcTypeControlAddress         = 2;   /* EEPROM address for "AC" type control */
    static const int EmergencyStopEnabledAddress  = 4;   /* EEPROM address for emergency stop option */
    static const int SelectedLocAddress           = 48;  /* EEPORM address for storage of selected locomotive. */
    static const int EepIpAddressZ21              = 165; /* EEPROM Address of IP address of Z21. */
    static const int locLibEepromAddressNumOfLocs = 181; /* EEPROM address num of locs. */
    static const int locLibEepromAddressData      = 185; /* Start in EEPROM address loc data. */
};
#endif
