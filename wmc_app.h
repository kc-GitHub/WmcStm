#ifndef WMC_APP_H
#define WMC_APP_H

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WmcTft.h>
#include <Z21Slave.h>
#include <tinyfsm.hpp>

// ----------------------------------------------------------------------------
// Event declarations
//

enum pulseSwitchStatus
{
    turn = 0,
    pushturn,
    pushedShort,
    pushedNormal,
    pushedlong,
};

struct pulseSwitchEvent : tinyfsm::Event
{
    int8_t Delta;
    pulseSwitchStatus Status;
};

enum pushButtons
{
    button_0 = 0,
    button_1,
    button_2,
    button_3,
    button_4,
    button_5,
    button_power,
    button_none
};

struct pushButtonsEvent : tinyfsm::Event
{
    pushButtons Button;
};

struct updateEvent50msec : tinyfsm::Event
{
};

struct updateEvent500msec : tinyfsm::Event
{
};

struct updateEvent3sec : tinyfsm::Event
{
};

// ----------------------------------------------------------------------------
// WMCr (FSM base class) declaration
//

class wmcApp : public tinyfsm::Fsm<wmcApp>
{
    /* NOTE: react(), entry() and exit() functions need to be accessible
     * from tinyfsm::Fsm class. You might as well declare friendship to
     * tinyfsm::Fsm, and make these functions private:
     *
     * friend class Fsm;
     */
public:
    /* default reaction for unhandled events */
    void react(tinyfsm::Event const&){};

    virtual void react(updateEvent3sec const&);
    virtual void react(pushButtonsEvent const&);
    virtual void react(pulseSwitchEvent const&);
    virtual void react(updateEvent50msec const&);
    virtual void react(updateEvent500msec const&);

    virtual void entry(void){}; /* entry actions in some states */
    virtual void exit(void){};  /* no exit actions at all */

protected:
    uint16_t limitLocAddress(uint16_t locAddress);
    Z21Slave::dataType WmcCheckForDataRx(void);
    void WmcCheckForDataTx(void);
    void updateLocInfoOnScreen(bool updateAll);
    void PrepareLanXSetLocoDriveAndTransmit(void);

    static WmcTft m_wmcTft;
    static LocLib m_locLib;
    static WiFiUDP m_WifiUdp;
    static bool m_TrackPower;
    static Z21Slave m_z21Slave;
    static bool m_locSelection;
    static uint16_t m_ConnectCnt;
    static uint8_t m_IpAddres[4];
    static uint16_t m_UdpLocalPort;
    static uint16_t m_locAddressAdd;
    static uint16_t m_TurnOutAddress;
    static Z21Slave::turnout m_TurnOutDirection;
    static uint32_t m_TurnoutOffDelay;
    static uint16_t m_locAddressChange;
    static uint16_t m_locAddressDelete;
    static byte m_WmcPacketBuffer[40];
    static uint8_t m_locFunctionAdd;
    static uint8_t m_locFunctionChange;
    static uint8_t m_locFunctionAssignment[5];
    static Z21Slave::locInfo m_WmcLocInfoControl;
    static Z21Slave::locInfo* m_WmcLocInfoReceived;

    static const uint8_t CONNECT_CNT_MAX_FAIL_CONNECT_WIFI = 120;
    static const uint8_t CONNECT_CNT_MAX_FAIL_CONNECT_UDP  = 10;
};

#endif