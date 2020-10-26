/**
 **********************************************************************************************************************
 * @file  wmc_event.h
 * @brief Collection of event data and events fo the WMC application.
 ***********************************************************************************************************************
 */
#ifndef WMC_EVENT_H
#define WMC_EVENT_H

/***********************************************************************************************************************
 * I N C L U D E S
 **********************************************************************************************************************/
#include <tinyfsm.hpp>
#include <app_cfg.h>

/***********************************************************************************************************************
 * T Y P E D  E F S  /  E N U M
 **********************************************************************************************************************/

/**
 * Status of the pulse switch.
 */
enum pulseSwitchStatus
{
    turn = 0,
    pushturn,
    pushedShort,
    pushedNormal,
    pushedlong,
};

/**
 * Definitions for the push buttons and mapping to key codes of the key pad matrix
 */
enum pushButtons
{
    button_0        = KEYCODE_0,
    button_1        = KEYCODE_1,
    button_2        = KEYCODE_2,
    button_3        = KEYCODE_3,
    button_4        = KEYCODE_4,
    button_5        = KEYCODE_5,
    button_6        = KEYCODE_6,
    button_7        = KEYCODE_7,
    button_8        = KEYCODE_8,
    button_9        = KEYCODE_9,
    button_left     = KEYCODE_LEFT,
    button_right    = KEYCODE_RIGHT,
    button_power    = KEYCODE_POWER,
    button_menu     = KEYCODE_MENU,
    button_mode     = KEYCODE_MODE,
    button_encoder  = KEYCODE_ENCODER_BTN,
    button_none     = 0
};

enum confirmationTypes
{
    deleteWiFiSettings = 0,
    deleteLocks,
    deleteAll
};


/**
 * CV programming module events.
 */
enum cvProgRequest
{
    cvRead = 0,
    cvWrite,
    cvStatusRequest,
    pomWrite,
    cvExit,
};

/**
 * Pulse switch event.
 */
struct pulseSwitchEvent : tinyfsm::Event
{
    int8_t Delta;             /* Delta of pulsw switch. */
    pulseSwitchStatus Status; /* Status */
};

/**
 * Event for buttons.
 */
struct pushButtonsEvent : tinyfsm::Event
{
    pushButtons Button;                 /* Button which was pressed. */
    confirmationTypes ConfirmationType; /* Saves type of confirmation */
};

/**
 * Event for powerOffButton.
 */
struct powerOffEvent : tinyfsm::Event
{
};

/**
 * 5msec Update event
 */
struct updateEvent5msec : tinyfsm::Event
{
};

/**
 * 50msec Update event
 */
struct updateEvent50msec : tinyfsm::Event
{
};

/**
 * 100msec Update event
 */
struct updateEvent100msec : tinyfsm::Event
{
};

/**
 * 500msec Update event
 */
struct updateEvent500msec : tinyfsm::Event
{
};

/**
 * 3 Seconds ipdate event
 */
struct updateEvent3sec : tinyfsm::Event
{
};

/**
 * Data changed by command line interface event.
 */
struct cliEnterEvent : tinyfsm::Event
{
};

/**
 * CV programming events from cv module.
 */
struct cvProgEvent : tinyfsm::Event
{
    cvProgRequest Request;
    uint16_t Address;
    uint16_t CvNumber;
    uint8_t CvValue;
};

/***********************************************************************************************************************
 * C L A S S E S
 **********************************************************************************************************************/

#endif
