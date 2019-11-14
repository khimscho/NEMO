/// \file StatusLED.h
/// \brief Interface for status LED management
///
/// The logger has status LEDs to indicate what it's doing, and things like a full
/// SD card, or bad card, etc.  This code manages the colours and specifics of
/// the system.
///
/// Copyright (c) 2019, University of New Hampshire, Center for Coastal and Ocean Mapping.
/// All Rights Reserved.

#ifndef __STATUS_LED_H__
#define __STATUS_LED_H__

#include <Arduino.h>

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
#define DEFAULT_RED_LED_PIN 25
#define DEFAULT_GREEN_LED_PIN 26
#define DEFAULT_BLUE_LED_PIN 27
#else
#define DEFAULT_RED_LED_PIN 8
#define DEFAULT_GREEN_LED_PIN 9
#define DEFAULT_BLUE_LED_PIN 22
#endif

class StatusLED {
public:
    StatusLED(int red_ping = DEFAULT_RED_LED_PIN,
              int green_pin = DEFAULT_GREEN_LED_PIN,
              int blue_pin = DEFAULT_BLUE_LED_PIN);
    
    enum Status {
        sINITIALISING,
        sNORMAL,
        sCARD_FULL,
        sFATAL_ERROR
    };
    
    /// \brief Set the status for the LED
    void SetStatus(Status status);
    
    /// \brief Switch state of LED if time period has expired
    void ProcessFlash(void);
    
private:
    int             led_pins[3];
    boolean         led_state[3];
    boolean         led_flasher;
    unsigned long   last_change_time;
    unsigned long   on_period;
    
    enum Colour {
        cINITIALISING,
        cNORMAL,
        cCARD_FULL,
        cALARM
    };
    
    void SetColour(Colour colour, boolean flash = false);
};

#endif
