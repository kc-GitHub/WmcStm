/***********************************************************************************************************************
   @file   wc_app.cpp
   @brief  Main application of WifiManualControl (WMC).
 **********************************************************************************************************************/

/***********************************************************************************************************************
   I N C L U D E S
 **********************************************************************************************************************/
#include "wmc_app.h"
#include "eep_cfg.h"
#include "fsmlist.hpp"
#include <EEPROM.h>
#include <tinyfsm.hpp>

/***********************************************************************************************************************
   D E F I N E S
 **********************************************************************************************************************/
#define WMC_APP_DEBUG_TX_RX 0

/***********************************************************************************************************************
   F O R W A R D  D E C L A R A T I O N S
 **********************************************************************************************************************/
class initUdpConnect;
class initUdpConnectFail;
class setUpWifiFail;
class initBroadcast;
class initStatusGet;
class initLocInfoGet;
class powerOff;
class powerOn;
class powerProgrammingMode;
class turnoutControl;
class turnoutControlPowerOff;
class mainMenu;
class menuLocAdd;
class menuLocFunctionsAdd;
class menuLocFunctionsChange;
class menuLocDelete;

/***********************************************************************************************************************
   D A T A   D E C L A R A T I O N S (exported, local)
 **********************************************************************************************************************/

/* Init variables. */
WmcTft wmcApp::m_wmcTft;
LocLib wmcApp::m_locLib;
WiFiUDP wmcApp::m_WifiUdp;
Z21Slave wmcApp::m_z21Slave;
bool wmcApp::m_locSelection;
uint8_t wmcApp::m_IpAddres[4];
bool wmcApp::m_TrackPower = false;
byte wmcApp::m_WmcPacketBuffer[40];
uint16_t wmcApp::m_ConnectCnt                = 0;
uint16_t wmcApp::m_UdpLocalPort              = 21105;
uint16_t wmcApp::m_locAddressAdd             = 1;
uint16_t wmcApp::m_TurnOutAddress            = ADDRESS_TURNOUT_MIN;
Z21Slave::turnout wmcApp::m_TurnOutDirection = Z21Slave::directionOff;
uint32_t wmcApp::m_TurnoutOffDelay           = 0;
uint8_t wmcApp::m_locFunctionAdd             = 0;
uint8_t wmcApp::m_locFunctionChange          = 0;
uint16_t wmcApp::m_locAddressDelete          = 0;
uint16_t wmcApp::m_locAddressChange          = 0;
bool wmcApp::m_WmcLocSpeedRequestPending     = false;
uint8_t wmcApp::m_locFunctionAssignment[5];
Z21Slave::locInfo wmcApp::m_WmcLocInfoControl;
Z21Slave::locInfo* wmcApp::m_WmcLocInfoReceived = NULL;

/***********************************************************************************************************************
  F U N C T I O N S
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Init the wifi connection.
 */
class setUpWifi : public wmcApp
{
    /**
     * Init modules and start connection to wifi network.
     */
    void entry() override
    {
        char SsidName[50];
        char SsidPassword[50];

        m_ConnectCnt = 0;

        m_wmcTft.Init();
        m_locLib.Init();
        m_wmcTft.UpdateStatus("Connecting to Wifi", true, WmcTft::color_yellow);
        m_wmcTft.WifiConnectUpdate(m_ConnectCnt);

        /* Get SSID data from EEPROM. */
        EEPROM.get(EepCfg::SsidAddress, SsidName);
        EEPROM.get(EepCfg::SsidPasswordAddress, SsidPassword);
        EEPROM.get(EepCfg::EepIpAddress, m_IpAddres);

        WiFi.mode(WIFI_STA);
        WiFi.begin(SsidName, SsidPassword);
    };

    /**
     * Wait for connection or when no connection can be made enter wifi error state.
     */
    void react(updateEvent500msec const&) override
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            m_ConnectCnt++;
            if (m_ConnectCnt < CONNECT_CNT_MAX_FAIL_CONNECT_WIFI)
            {
                m_wmcTft.WifiConnectUpdate(m_ConnectCnt);
            }
            else
            {
                transit<setUpWifiFail>();
            }
        }
        else
        {
            /* Start UDP */
            transit<initUdpConnect>();
        }
    };

    /**
     * Avoid sending data.
     */
    void react(updateEvent3sec const&) override{};
};

/***********************************************************************************************************************
 * No wifi connection could be made, show error screen.
 */
class setUpWifiFail : public wmcApp
{
    void entry() override { m_wmcTft.WifiConnectFailed(); }
};

/***********************************************************************************************************************
 * Setup udp connection and request status to get communication up and running.
 */
class initUdpConnect : public wmcApp
{
    /**
     * Start UDP connection.
     */
    void entry()
    {
        m_ConnectCnt = 0;
        m_wmcTft.UpdateStatus("Connect to Control", true, WmcTft::color_yellow);
        m_wmcTft.WifiConnectUpdate(m_ConnectCnt);
        m_WifiUdp.begin(m_UdpLocalPort);
    }

    /**
     * Request status to check connection with control.
     */
    void react(updateEvent500msec const&) override
    {
        m_ConnectCnt++;

        if (m_ConnectCnt < CONNECT_CNT_MAX_FAIL_CONNECT_UDP)
        {
            m_z21Slave.LanGetStatus();
            WmcCheckForDataTx();
            m_wmcTft.WifiConnectUpdate(m_ConnectCnt);
        }
        else
        {
            transit<initUdpConnectFail>();
        }
    };

    /**
     * Handle the response on the status message.
     */
    void react(updateEvent50msec const&) override
    {
        switch (WmcCheckForDataRx())
        {
        case Z21Slave::trackPowerOff:
        case Z21Slave::programmingMode:
        case Z21Slave::trackPowerOn: transit<initBroadcast>(); break;
        default: break;
        }
    };

    /**
     * Override update during init.
     */
    void react(updateEvent3sec const&) override{};
};

/***********************************************************************************************************************
 * No UDP connection to control unit possible.
 */
class initUdpConnectFail : public wmcApp
{
    void entry() override { m_wmcTft.UdpConnectFailed(); }
};

/***********************************************************************************************************************
 * Sent broadcast message.
 */
class initBroadcast : public wmcApp
{
    /**
     * Transmit broadcast info.
     */
    void entry() override
    {
        m_z21Slave.LanSetBroadCastFlags(1);
        WmcCheckForDataTx();
    };

    /**
     * Continue to next state.
     */
    void react(updateEvent50msec const&) override { transit<initStatusGet>(); };

    /**
     * Override update during init.
     */
    void react(updateEvent3sec const&) override{};
};

/***********************************************************************************************************************
 * Get the status of the control unit.
 */
class initStatusGet : public wmcApp
{
    /**
     * Get the status.
     */
    void entry() override
    {
        m_z21Slave.LanGetStatus();
        WmcCheckForDataTx();
    };

    /**
     * Check response of status request.
     */
    void react(updateEvent50msec const&) override
    {
        switch (WmcCheckForDataRx())
        {
        case Z21Slave::trackPowerOff:
            m_TrackPower = false;
            transit<initLocInfoGet>();
            break;
        case Z21Slave::programmingMode:
            m_TrackPower = false;
            transit<initLocInfoGet>();
            break;
        case Z21Slave::trackPowerOn:
            m_TrackPower = true;
            transit<initLocInfoGet>();
            break;
        default: break;
        }
    };

    /**
     * No response, retry.
     */
    void react(updateEvent500msec const&) override { m_z21Slave.LanGetStatus(); };

    /**
     * Override update during init.
     */
    void react(updateEvent3sec const&) override{};
};

/***********************************************************************************************************************
 * Get the data of the actual selected loc.
 */
class initLocInfoGet : public wmcApp
{
    /**
     * Request loc info.
     */
    void entry() override
    {
        m_z21Slave.LanXGetLocoInfo(m_locLib.GetActualLocAddress());
        WmcCheckForDataTx();
    };

    /**
     * Handle response of loc request and if loc data received setup screen.
     */
    void react(updateEvent50msec const&) override
    {
        switch (WmcCheckForDataRx())
        {
        case Z21Slave::locinfo:
            m_wmcTft.Clear();
            updateLocInfoOnScreen(true);
            m_locLib.SpeedUpdate(m_WmcLocInfoReceived->Speed);
            if (m_TrackPower == false)
            {
                transit<powerOff>();
            }
            else
            {
                transit<powerOn>();
            }
            break;
        default: break;
        }
    };

    /**
     * No response, retry.
     */
    void react(updateEvent500msec const&) override
    {
        m_z21Slave.LanXGetLocoInfo(m_locLib.GetActualLocAddress());
        WmcCheckForDataTx();
    }

    /**
     * Override update during init.
     */
    void react(updateEvent3sec const&) override{};
};

/***********************************************************************************************************************
 * Control in power off mode. From here go to power on or menu.
 */
class powerOff : public wmcApp
{
    uint8_t Index = 0;

    /**
     * Update status row.
     */
    void entry() override
    {
        m_locSelection = false;
        m_wmcTft.UpdateStatus("PowerOff", false, WmcTft::color_red);
        m_wmcTft.UpdateSelectedAndNumberOfLocs(m_locLib.GetActualSelectedLocIndex(), m_locLib.GetNumberOfLocs());
    }

    /**
     * Handle received data.
     */
    void react(updateEvent50msec const&) override
    {
        switch (WmcCheckForDataRx())
        {
        case Z21Slave::trackPowerOn: transit<powerOn>(); break;
        case Z21Slave::programmingMode: transit<powerProgrammingMode>(); break;
        case Z21Slave::locinfo:
            updateLocInfoOnScreen(false);
            m_locLib.SpeedUpdate(m_WmcLocInfoReceived->Speed);
            break;
        default: break;
        }
    }

    /**
     * Keep alive.
     */
    void react(updateEvent3sec const&) override
    {
        m_z21Slave.LanXGetLocoInfo(m_locLib.GetActualLocAddress());
        WmcCheckForDataTx();
    }

    /**
     * Check button event data.
     */
    void react(pushButtonsEvent const& e) override
    {
        switch (e.Button)
        {
        case button_power:
            /* Power on request. */
            m_z21Slave.LanSetTrackPowerOn();
            WmcCheckForDataTx();
            break;
        default: break;
        }
    }

    /**
     * Check pulse switch event data.
     */
    void react(pulseSwitchEvent const& e) override
    {
        switch (e.Status)
        {
        case pushturn:
            /* Select next or previous loc. */
            if (e.Delta != 0)
            {
                m_locLib.GetNextLoc(e.Delta);
                m_z21Slave.LanXGetLocoInfo(m_locLib.GetActualLocAddress());
                WmcCheckForDataTx();
                m_wmcTft.UpdateSelectedAndNumberOfLocs(
                    m_locLib.GetActualSelectedLocIndex(), m_locLib.GetNumberOfLocs());
                m_locSelection = true;
            }
            break;
        case pushedlong: transit<mainMenu>(); break;
        default: break;
        }
    }
};

/***********************************************************************************************************************
 * Control is on, control the loc speed and functions, go back to power off or select another locomotive.
 */
class powerOn : public wmcApp
{
    /**
     * Update status row.
     */
    void entry() override
    {
        m_locSelection              = false;
        m_WmcLocSpeedRequestPending = false;
        m_wmcTft.UpdateStatus("PowerOn", false, WmcTft::color_green);
        m_wmcTft.UpdateSelectedAndNumberOfLocs(m_locLib.GetActualSelectedLocIndex(), m_locLib.GetNumberOfLocs());
    };

    /**
     * Handle received data.
     */
    void react(updateEvent50msec const&) override
    {
        switch (WmcCheckForDataRx())
        {
        case Z21Slave::trackPowerOff: transit<powerOff>(); break;
        case Z21Slave::programmingMode: transit<powerProgrammingMode>(); break;
        case Z21Slave::locinfo:
            updateLocInfoOnScreen(false);
            m_WmcLocSpeedRequestPending = false;
            m_locLib.SpeedUpdate(m_WmcLocInfoReceived->Speed);
            break;
        default: break;
        }
    };

    /**
     * Keep alive.
     */
    void react(updateEvent3sec const&) override
    {
        m_z21Slave.LanXGetLocoInfo(m_locLib.GetActualLocAddress());
        m_WmcLocSpeedRequestPending = true;
        WmcCheckForDataTx();
    }

    /**
     * Handle pulse switch events.
     */
    void react(pulseSwitchEvent const& e) override
    {
        switch (e.Status)
        {
        case pushturn:
            /* Select next or previous loc. */
            if (e.Delta != 0)
            {
                m_locLib.GetNextLoc(e.Delta);
                m_wmcTft.UpdateSelectedAndNumberOfLocs(
                    m_locLib.GetActualSelectedLocIndex(), m_locLib.GetNumberOfLocs());
                m_z21Slave.LanXGetLocoInfo(m_locLib.GetActualLocAddress());
                WmcCheckForDataTx();
                m_locSelection = true;
            }
            break;
        case turn:
            /* Increase or decrease speed. */
            if (m_WmcLocSpeedRequestPending == false)
            {
                if (m_locLib.SpeedSet(e.Delta) == true)
                {
                    m_WmcLocSpeedRequestPending = true;
                    PrepareLanXSetLocoDriveAndTransmit();
                }
            }
            break;
        case pushedShort:
            /* Stop or change direction when speed is zero. */
            if (m_locLib.SpeedSet(0) == true)
            {
                PrepareLanXSetLocoDriveAndTransmit();
            }
            break;
        case pushedNormal:
            /* Change direction. */
            m_locLib.DirectionToggle();
            PrepareLanXSetLocoDriveAndTransmit();
            break;
        default: break;
        }
    };

    /**
     * Handle button events.
     */
    void react(pushButtonsEvent const& e) override
    {
        uint8_t Function = 0;
        switch (e.Button)
        {
        case button_power:
            m_z21Slave.LanSetTrackPowerOff();
            WmcCheckForDataTx();
            break;
        case button_0:
        case button_1:
        case button_2:
        case button_3:
        case button_4:
            Function = m_locLib.FunctionAssignedGet(static_cast<uint8_t>(e.Button));
            m_locLib.FunctionToggle(Function);
            if (m_locLib.FunctionStatusGet(Function) == LocLib::functionOn)
            {
                m_z21Slave.LanXSetLocoFunction(m_locLib.GetActualLocAddress(), Function, Z21Slave::on);
            }
            else
            {
                m_z21Slave.LanXSetLocoFunction(m_locLib.GetActualLocAddress(), Function, Z21Slave::off);
            }
            WmcCheckForDataTx();
            break;
        case button_5:
            m_wmcTft.Clear();
            transit<turnoutControl>();
            break;
        default: break;
        }
    };
};

/***********************************************************************************************************************
 * Another Z21 device is in programming mode, do nothing....
 */
class powerProgrammingMode : public wmcApp
{
    /**
     * Update status row.
     */
    void entry() override
    {
        m_locSelection = false;
        m_wmcTft.UpdateStatus("ProgMode", false, WmcTft::color_red);
        m_wmcTft.UpdateSelectedAndNumberOfLocs(m_locLib.GetActualSelectedLocIndex(), m_locLib.GetNumberOfLocs());
    };

    /**
     * Handle received data.
     */
    void react(updateEvent50msec const&) override
    {
        switch (WmcCheckForDataRx())
        {
        case Z21Slave::trackPowerOff: transit<powerOff>(); break;
        case Z21Slave::trackPowerOn: transit<powerOn>(); break;
        default: break;
        }
    };

    /**
     * Handle button events.
     */
    void react(pushButtonsEvent const& e) override
    {
        switch (e.Button)
        {
        case button_power:
            m_z21Slave.LanSetTrackPowerOff();
            WmcCheckForDataTx();
            break;
        default: break;
        }
    };
};

/***********************************************************************************************************************
 * Show main menu and handle the request.
 */
class turnoutControl : public wmcApp
{
    /**
     * Show turnout screen.
     */
    void entry() override
    {
        m_TurnOutDirection = Z21Slave::directionOff;

        m_wmcTft.UpdateStatus("TURNOUT", true, WmcTft::color_green);
        m_wmcTft.ShowTurnoutScreen();
        m_wmcTft.ShowTurnoutAddress(m_TurnOutAddress);
        m_wmcTft.ShowTurnoutDirection(static_cast<uint8_t>(m_TurnOutDirection));
    };

    /**
     * Handle received data.
     */
    void react(updateEvent50msec const&) override
    {
        switch (WmcCheckForDataRx())
        {
        case Z21Slave::trackPowerOff: transit<turnoutControlPowerOff>(); break;
        default: break;
        }

        /* When turnout active sent after 500msec off command. */
        if (m_TurnOutDirection != Z21Slave::directionOff)
        {
            if ((millis() - m_TurnoutOffDelay) > 500)
            {
                m_TurnOutDirection = Z21Slave::directionOff;
                m_z21Slave.LanXSetTurnout(m_TurnOutAddress - 1, m_TurnOutDirection);
                WmcCheckForDataTx();
                m_wmcTft.ShowTurnoutDirection(static_cast<uint8_t>(m_TurnOutDirection));
            }
        }
    };

    /**
     * Handle pulse switch events.
     */
    void react(pulseSwitchEvent const& e) override
    {
        bool updateScreen = false;
        switch (e.Status)
        {
        case pushturn: break;
        case turn:
            if (e.Delta > 0)
            {
                /* Increase address and check for overrrun. */
                if (m_TurnOutAddress < ADDRESS_TURNOUT_MAX)
                {
                    m_TurnOutAddress++;
                }
                else
                {
                    m_TurnOutAddress = ADDRESS_TURNOUT_MIN;
                }

                updateScreen = true;
            }
            else if (e.Delta < 0)
            {
                /* Decrease address and handle address 0. */
                if (m_TurnOutAddress > ADDRESS_TURNOUT_MIN)
                {
                    m_TurnOutAddress--;
                }
                else
                {
                    m_TurnOutAddress = ADDRESS_TURNOUT_MAX;
                }
                updateScreen = true;
            }
            break;
        case pushedShort:
            /* Reset turnout address. */
            m_TurnOutAddress = ADDRESS_TURNOUT_MIN;
            updateScreen     = true;
            break;
        case pushedNormal:
            /* Back to loc control. */
            transit<initStatusGet>();
            break;
        default: break;
        }

        /* Update address on display if required. */
        if (updateScreen == true)
        {
            m_wmcTft.ShowTurnoutAddress(m_TurnOutAddress);
        }
    };

    /**
     * Handle button events.
     */
    void react(pushButtonsEvent const& e) override
    {
        bool updateScreen       = true;
        bool sentTurnOutCommand = false;

        /* Handle button requests. */
        switch (e.Button)
        {
        case button_power:
            m_z21Slave.LanSetTrackPowerOff();
            WmcCheckForDataTx();
            break;
        case button_0: m_TurnOutAddress++; break;
        case button_1: m_TurnOutAddress += 10; break;
        case button_2: m_TurnOutAddress += 100; break;
        case button_3: m_TurnOutAddress += 1000; break;
        case button_4:
            m_TurnOutDirection = Z21Slave::directionForward;
            m_TurnoutOffDelay  = millis();
            updateScreen       = false;
            sentTurnOutCommand = true;
            break;
        case button_5:
            m_TurnOutDirection = Z21Slave::directionTurn;
            m_TurnoutOffDelay  = millis();
            updateScreen       = false;
            sentTurnOutCommand = true;
            break;
        default: break;
        }

        if (updateScreen == true)
        {
            if (m_TurnOutAddress > ADDRESS_TURNOUT_MAX)
            {
                m_TurnOutAddress = 1;
            }
            m_wmcTft.ShowTurnoutAddress(m_TurnOutAddress);
        }

        if (sentTurnOutCommand == true)
        {
            /* Sent command and show turnout direction. */
            m_z21Slave.LanXSetTurnout(m_TurnOutAddress - 1, m_TurnOutDirection);
            WmcCheckForDataTx();
            m_wmcTft.ShowTurnoutDirection(static_cast<uint8_t>(m_TurnOutDirection));
        }
    };

    /**
     * When exit and turnout active transmit off command.
     */
    void exit() override
    {
        if (m_TurnOutDirection != Z21Slave::directionOff)
        {
            m_TurnOutDirection = Z21Slave::directionOff;
            m_z21Slave.LanXSetTurnout(m_TurnOutAddress - 1, m_TurnOutDirection);
            WmcCheckForDataTx();
            m_wmcTft.ShowTurnoutDirection(static_cast<uint8_t>(m_TurnOutDirection));
        }
    }
};

/***********************************************************************************************************************
 * Power off screen and handling for turnout control.
 */
class turnoutControlPowerOff : public wmcApp
{
    /**
     * Show turnout screen.
     */
    void entry() override
    {
        m_wmcTft.UpdateStatus("TURNOUT", true, WmcTft::color_red);
        m_TrackPower = false;
    };

    /**
     * Handle received data.
     */
    void react(updateEvent50msec const&) override
    {
        switch (WmcCheckForDataRx())
        {
        case Z21Slave::trackPowerOff: break;
        case Z21Slave::trackPowerOn:
            m_TrackPower = true;
            transit<turnoutControl>();
            break;
        default: break;
        }
    };

    /**
     * Handle pulse switch events.
     */
    void react(pulseSwitchEvent const& e) override
    {
        switch (e.Status)
        {
        case pushturn: break;
        case turn:
        case pushedShort: break;
        case pushedNormal:
            /* Back to loc control. */
            transit<initLocInfoGet>();
            break;
        default: break;
        }
    };

    /**
     * Handle button events.
     */
    void react(pushButtonsEvent const& e) override
    {
        /* Handle button requests. */
        switch (e.Button)
        {
        case button_power:
            m_z21Slave.LanSetTrackPowerOn();
            WmcCheckForDataTx();
            break;
        case button_0:
        case button_1:
        case button_2:
        case button_3:
        case button_4:
        case button_5: break;
        default: break;
        }
    };
};

/***********************************************************************************************************************
 * Show main menu and handle the request.
 */
class mainMenu : public wmcApp
{
    /**
     * Show menu on screen.
     */
    void entry() override { m_wmcTft.ShowMenu(); };

    /**
     * Handle switch events.
     */
    void react(pushButtonsEvent const& e) override
    {
        /* Handle menu request. */
        switch (e.Button)
        {
        case button_1: transit<menuLocAdd>(); break;
        case button_2: transit<menuLocFunctionsChange>(); break;
        case button_3: transit<menuLocDelete>(); break;
        case button_4:
        case button_power: transit<initLocInfoGet>(); break;
        default: break;
        }
    };
};

/***********************************************************************************************************************
 * Add a loc.
 */
class menuLocAdd : public wmcApp
{
    /**
     * Show loc menu add screen.
     */
    void entry() override
    {
        m_locAddressAdd = m_locLib.GetActualLocAddress();

        // Show loc add screen.
        m_wmcTft.Clear();
        m_wmcTft.UpdateStatus("ADD LOC", true, WmcTft::color_green);
        m_wmcTft.ShowLocSymbolFw(WmcTft::color_white);
        m_wmcTft.ShowlocAddress(m_locAddressAdd, WmcTft::color_green);
        m_wmcTft.UpdateSelectedAndNumberOfLocs(m_locLib.GetActualSelectedLocIndex(), m_locLib.GetNumberOfLocs());
    };

    /**
     * Handle pulse switch events.
     */
    void react(pulseSwitchEvent const& e) override
    {
        switch (e.Status)
        {
        case turn:
            /* Increase or decrease loc address to be added. */
            if (e.Delta > 0)
            {
                m_locAddressAdd++;
                m_locAddressAdd = limitLocAddress(m_locAddressAdd);
                m_wmcTft.ShowlocAddress(m_locAddressAdd, WmcTft::color_green);
            }
            else if (e.Delta < 0)
            {
                m_locAddressAdd--;
                m_locAddressAdd = limitLocAddress(m_locAddressAdd);
                m_wmcTft.ShowlocAddress(m_locAddressAdd, WmcTft::color_green);
            }
            break;
        case pushturn: break;
        case pushedNormal:
        case pushedlong:
            /* If loc is not present goto add functions else red address indicating loc already present. */
            if (m_locLib.CheckLoc(m_locAddressAdd) != 255)
            {
                m_wmcTft.ShowlocAddress(m_locAddressAdd, WmcTft::color_red);
            }
            else
            {
                transit<menuLocFunctionsAdd>();
            }
            break;
        default: break;
        }
    };

    /**
     * Handle button events for easier / faster increase of address and reset of address.
     */
    void react(pushButtonsEvent const& e) override
    {
        bool updateScreen = true;

        switch (e.Button)
        {
        case button_0: m_locAddressAdd = 1; break;
        case button_1: m_locAddressAdd++; break;
        case button_2: m_locAddressAdd += 10; break;
        case button_3: m_locAddressAdd += 100; break;
        case button_4: m_locAddressAdd += 1000; break;
        case button_power:
            updateScreen = false;
            transit<mainMenu>();
            break;
        case button_5:
        default: updateScreen = false; break;
        }

        if (updateScreen == true)
        {
            m_locAddressAdd = limitLocAddress(m_locAddressAdd);
            m_wmcTft.ShowlocAddress(m_locAddressAdd, WmcTft::color_green);
        }
    };
};

/***********************************************************************************************************************
 * Set the functions of the loc to be added.
 */
class menuLocFunctionsAdd : public wmcApp
{
    /**
     * Show function add screen.
     */
    void entry() override
    {
        uint8_t Index;

        m_wmcTft.UpdateStatus("FUNCTIONS", true, WmcTft::color_green);
        m_locFunctionAdd = 0;
        for (Index = 0; Index < 5; Index++)
        {
            m_locFunctionAssignment[Index] = Index;
        }

        m_wmcTft.FunctionAddSet();
        m_wmcTft.UpdateSelectedAndNumberOfLocs(m_locLib.GetActualSelectedLocIndex(), m_locLib.GetNumberOfLocs());
    };

    /**
     * Handle pulse switch events.
     */
    void react(pulseSwitchEvent const& e) override
    {
        switch (e.Status)
        {
        case turn:
            /* ncrease of decrease the function. */
            if (e.Delta > 0)
            {
                m_locFunctionAdd++;
                if (m_locFunctionAdd > FUNCTION_MAX)
                {
                    m_locFunctionAdd = FUNCTION_MIN;
                }
                m_wmcTft.FunctionAddUpdate(m_locFunctionAdd);
            }
            else if (e.Delta > 0)
            {
                if (m_locFunctionAdd == FUNCTION_MIN)
                {
                    m_locFunctionAdd = FUNCTION_MAX;
                }
                else
                {
                    m_locFunctionAdd--;
                }
                m_wmcTft.FunctionAddUpdate(m_locFunctionAdd);
            }
            break;
        case pushedNormal:
            /* Store loc functions */
            m_locLib.StoreLoc(m_locAddressAdd, m_locFunctionAssignment, LocLib::storeAdd);
            m_locLib.LocBubbleSort();
            m_locAddressAdd++;
            transit<menuLocAdd>();
            break;
        default: break;
        }
    };

    /**
     * Handle button events.
     */
    void react(pushButtonsEvent const& e) override
    {
        switch (e.Button)
        {
        case button_0:
            /* Button 0 only for light or other functions. */
            m_locFunctionAssignment[static_cast<uint8_t>(e.Button)] = m_locFunctionAdd;
            m_wmcTft.UpdateFunction(
                static_cast<uint8_t>(e.Button), m_locFunctionAssignment[static_cast<uint8_t>(e.Button)]);
            break;
        case button_1:
        case button_2:
        case button_3:
        case button_4:
            /* Rest of buttons for oher functions except light. */
            if (m_locFunctionAdd != 0)
            {
                m_locFunctionAssignment[static_cast<uint8_t>(e.Button)] = m_locFunctionAdd;
                m_wmcTft.UpdateFunction(
                    static_cast<uint8_t>(e.Button), m_locFunctionAssignment[static_cast<uint8_t>(e.Button)]);
            }
            break;
        case button_power: transit<mainMenu>(); break;
        default: break;
        }
    };
};

/***********************************************************************************************************************
 * Changer functions of a loc already present.
 */
class menuLocFunctionsChange : public wmcApp
{
    /**
     * Show change function screen.
     */
    void entry() override
    {
        uint8_t Index;

        m_wmcTft.Clear();
        m_locFunctionChange = 0;
        m_locAddressChange  = m_locLib.GetActualLocAddress();
        m_wmcTft.UpdateStatus("CHANGE FUNC", true, WmcTft::color_green);
        m_wmcTft.ShowLocSymbolFw(WmcTft::color_white);
        m_wmcTft.ShowlocAddress(m_locAddressChange, WmcTft::color_green);
        m_wmcTft.FunctionAddUpdate(m_locFunctionChange);
        m_wmcTft.UpdateSelectedAndNumberOfLocs(m_locLib.GetActualSelectedLocIndex(), m_locLib.GetNumberOfLocs());

        for (Index = 0; Index < 5; Index++)
        {
            m_locFunctionAssignment[Index] = m_locLib.FunctionAssignedGet(Index);
            m_wmcTft.UpdateFunction(Index, m_locFunctionAssignment[Index]);
        }
    }

    /**
     * Handle pulse switch events.
     */
    void react(pulseSwitchEvent const& e) override
    {
        uint8_t Index = 0;

        switch (e.Status)
        {
        case turn:
            /* Change function. */
            if (e.Delta > 0)
            {
                m_locFunctionChange++;
                if (m_locFunctionChange > 28)
                {
                    m_locFunctionChange = 0;
                }
                m_wmcTft.FunctionAddUpdate(m_locFunctionChange);
            }
            else if (e.Delta < 0)
            {
                if (m_locFunctionChange == 0)
                {
                    m_locFunctionChange = 28;
                }
                else
                {
                    m_locFunctionChange--;
                }
                m_wmcTft.FunctionAddUpdate(m_locFunctionChange);
            }
            break;
        case pushturn:
            /* Select another loc and update function data of newly selected loc. */
            m_locAddressChange = m_locLib.GetNextLoc(e.Delta);
            m_wmcTft.UpdateSelectedAndNumberOfLocs(m_locLib.GetActualSelectedLocIndex(), m_locLib.GetNumberOfLocs());

            for (Index = 0; Index < 5; Index++)
            {
                m_locFunctionAssignment[Index] = m_locLib.FunctionAssignedGet(Index);
                m_wmcTft.UpdateFunction(Index, m_locFunctionAssignment[Index]);
            }

            m_wmcTft.ShowlocAddress(m_locAddressChange, WmcTft::color_green);
            break;
        case pushedNormal:
        case pushedlong:
            /* Store changed data and yellow text indicating data is stored. */
            m_locLib.StoreLoc(m_locAddressChange, m_locFunctionAssignment, LocLib::storeChange);
            m_wmcTft.ShowlocAddress(m_locAddressChange, WmcTft::color_yellow);
            break;
        default: break;
        }
    };

    /**
     * Handle button events.
     */
    void react(pushButtonsEvent const& e) override
    {
        switch (e.Button)
        {
        case button_0:
            /* Button 0 only for light or other functions. */
            m_locFunctionAssignment[static_cast<uint8_t>(e.Button)] = m_locFunctionChange;
            m_wmcTft.UpdateFunction(
                static_cast<uint8_t>(e.Button), m_locFunctionAssignment[static_cast<uint8_t>(e.Button)]);
            break;
        case button_1:
        case button_2:
        case button_3:
        case button_4:
            /* Rest of buttons for oher functions except light. */
            if (m_locFunctionChange != 0)
            {
                m_locFunctionAssignment[static_cast<uint8_t>(e.Button)] = m_locFunctionChange;
                m_wmcTft.UpdateFunction(
                    static_cast<uint8_t>(e.Button), m_locFunctionAssignment[static_cast<uint8_t>(e.Button)]);
            }
            break;
        case button_power: transit<mainMenu>(); break;
        default: break;
        }
    };
};

/***********************************************************************************************************************
 * Delete a loc.
 */
class menuLocDelete : public wmcApp
{
    /**
     * Show delete screen.
     */
    void entry() override
    {
        m_wmcTft.Clear();
        m_locAddressDelete = m_locLib.GetActualLocAddress();
        m_wmcTft.UpdateStatus("DELETE", true, WmcTft::color_green);
        m_wmcTft.ShowLocSymbolFw(WmcTft::color_white);
        m_wmcTft.ShowlocAddress(m_locAddressDelete, WmcTft::color_green);
        m_wmcTft.UpdateSelectedAndNumberOfLocs(m_locLib.GetActualSelectedLocIndex(), m_locLib.GetNumberOfLocs());
    }

    /**
     * Handle pulse switch events.
     */
    void react(pulseSwitchEvent const& e) override
    {
        switch (e.Status)
        {
        case turn:
            /* Select loc to be deleted. */
            m_locAddressDelete = m_locLib.GetNextLoc(e.Delta);
            m_wmcTft.UpdateSelectedAndNumberOfLocs(m_locLib.GetActualSelectedLocIndex(), m_locLib.GetNumberOfLocs());
            m_wmcTft.ShowlocAddress(m_locAddressDelete, WmcTft::color_green);
            break;
        case pushedNormal:
        case pushedlong:
            /* Remove loc. */
            m_locLib.RemoveLoc(m_locAddressDelete);
            m_wmcTft.UpdateSelectedAndNumberOfLocs(m_locLib.GetActualSelectedLocIndex(), m_locLib.GetNumberOfLocs());
            m_locAddressDelete = m_locLib.GetActualLocAddress();
            m_wmcTft.ShowlocAddress(m_locAddressDelete, WmcTft::color_green);
            break;
        default: break;
        }
    }

    /**
     * Handle button events, just go back to main menu on each button.
     */
    void react(pushButtonsEvent const& e) override
    {
        switch (e.Button)
        {
        case button_0:
        case button_1:
        case button_2:
        case button_3:
        case button_4:
        case button_power: transit<mainMenu>(); break;
        default: break;
        }
    };
};

/***********************************************************************************************************************
 * Default event handlers when not declared in states itself.
 */

void wmcApp::react(updateEvent50msec const&) { WmcCheckForDataRx(); };
void wmcApp::react(pulseSwitchEvent const&){};
void wmcApp::react(pushButtonsEvent const&){};
void wmcApp::react(updateEvent500msec const&){};
void wmcApp::react(updateEvent3sec const&)
{
    m_z21Slave.LanGetStatus();
    WmcCheckForDataTx();
};

/***********************************************************************************************************************
 * Initial state.
 */
FSM_INITIAL_STATE(wmcApp, setUpWifi)

/***********************************************************************************************************************
 * limit maximum loc addres.
 */
uint16_t wmcApp::limitLocAddress(uint16_t locAddress)
{
    uint16_t locAdrresReturn = locAddress;
    if (locAdrresReturn > ADDRESS_LOC_MAX)
    {
        locAdrresReturn = ADDRESS_LOC_MIN;
    }
    else if (locAdrresReturn == 0)
    {
        locAdrresReturn = ADDRESS_LOC_MAX;
    }

    return (locAdrresReturn);
}

/***********************************************************************************************************************
 * Check for received Z21 data and process it.
 */
Z21Slave::dataType wmcApp::WmcCheckForDataRx(void)
{
    int WmcPacketBufferLength     = 0;
    Z21Slave::dataType returnData = Z21Slave::none;
#if WMC_APP_DEBUG_TX_RX == 1
    uint8_t Index;
#endif

    if (m_WifiUdp.parsePacket())
    {
        // We've received a packet, read the data from it into the buffer
        WmcPacketBufferLength = m_WifiUdp.read(m_WmcPacketBuffer, 40);

        if (WmcPacketBufferLength != 0)
        {
#if WMC_APP_DEBUG_TX_RX == 1
            Serial.print("RX : ");

            for (Index = 0; Index < WmcPacketBufferLength; Index++)
            {
                Serial.print(m_WmcPacketBuffer[Index], HEX);
                Serial.print(" ");
            }

            Serial.println("");
#endif
            // Process the data.
            returnData = m_z21Slave.ProcesDataRx(m_WmcPacketBuffer, sizeof(m_WmcPacketBuffer));
        }
    }

    return (returnData);
}

/***********************************************************************************************************************
 * Check for data to be transmitted.
 */
void wmcApp::WmcCheckForDataTx(void)
{
    uint8_t* DataTransmitPtr;
    IPAddress WmcUdpIp(m_IpAddres[0], m_IpAddres[1], m_IpAddres[2], m_IpAddres[3]);

#if WMC_APP_DEBUG_TX_RX == 1
    uint8_t Index;
#endif

    if (m_z21Slave.txDataPresent() == true)
    {
        DataTransmitPtr = m_z21Slave.GetDataTx();

#if WMC_APP_DEBUG_TX_RX == 1
        Serial.print("TX : ");

        for (Index = 1; Index < DataTransmitPtr[0]; Index++)
        {
            Serial.print(DataTransmitPtr[Index], HEX);
            Serial.print(" ");
        }

        Serial.println("");
#endif

        m_WifiUdp.beginPacket(WmcUdpIp, m_UdpLocalPort);
        m_WifiUdp.write(DataTransmitPtr, DataTransmitPtr[0]);
        m_WifiUdp.endPacket();
    }
}

/***********************************************************************************************************************
 * Update loc info on screen.
 */
void wmcApp::updateLocInfoOnScreen(bool updateAll)
{
    uint8_t Index        = 0;
    m_WmcLocInfoReceived = m_z21Slave.LanXLocoInfo();

    if (m_locLib.GetActualLocAddress() == m_WmcLocInfoReceived->Address)
    {
        switch (m_WmcLocInfoReceived->Steps)
        {
        case Z21Slave::locDecoderSpeedSteps14: m_locLib.DecoderStepsUpdate(LocLib::decoderStep14); break;
        case Z21Slave::locDecoderSpeedSteps28: m_locLib.DecoderStepsUpdate(LocLib::decoderStep28); break;
        case Z21Slave::locDecoderSpeedSteps128: m_locLib.DecoderStepsUpdate(LocLib::decoderStep128); break;
        case Z21Slave::locDecoderSpeedStepsUnknown: m_locLib.DecoderStepsUpdate(LocLib::decoderStep28); break;
        }

        for (Index = 0; Index < 5; Index++)
        {
            m_locFunctionAssignment[Index] = m_locLib.FunctionAssignedGet(Index);
        }

        /* Invert functions so function symbols are updated if new loc is selected. */
        if (m_locSelection == true)
        {
            m_WmcLocInfoControl.Functions = ~m_WmcLocInfoReceived->Functions;
            m_locSelection                = false;
        }

        m_wmcTft.UpdateLocInfo(m_WmcLocInfoReceived, &m_WmcLocInfoControl, m_locFunctionAssignment, updateAll);

        memcpy(&m_WmcLocInfoControl, m_WmcLocInfoReceived, sizeof(Z21Slave::locInfo));
    }
}

/***********************************************************************************************************************
 * Compose locomotive message to be transmitted and transmit it.
 */
void wmcApp::PrepareLanXSetLocoDriveAndTransmit(void)
{
    Z21Slave::locInfo LocInfoTx;

    /* Get loc data and compose it for transmit */
    LocInfoTx.Speed = m_locLib.SpeedGet();
    if (m_locLib.DirectionGet() == LocLib::directionForward)
    {
        LocInfoTx.Direction = Z21Slave::locDirectionForward;
    }
    else
    {
        LocInfoTx.Direction = Z21Slave::locDirectionBackward;
    }

    LocInfoTx.Address = m_locLib.GetActualLocAddress();

    switch (m_locLib.DecoderStepsGet())
    {
    case LocLib::decoderStep14: LocInfoTx.Steps = Z21Slave::locDecoderSpeedSteps14; break;
    case LocLib::decoderStep28: LocInfoTx.Steps = Z21Slave::locDecoderSpeedSteps28; break;
    case LocLib::decoderStep128: LocInfoTx.Steps = Z21Slave::locDecoderSpeedSteps128; break;
    }

    m_z21Slave.LanXSetLocoDrive(&LocInfoTx);
    WmcCheckForDataTx();
}
