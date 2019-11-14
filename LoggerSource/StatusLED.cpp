/// \file StatusLED.cpp
/// \brief Implementation of status LED management
///
/// The logger has status LEDs to indicate what it's doing, and things like a full
/// SD card, or bad card, etc.  This code manages the colours and specifics of
/// the system.
///
/// Copyright (c) 2019, University of New Hampshire, Center for Coastal and Ocean Mapping.
/// All Rights Reserved.

#include "StatusLED.h"

const boolean ON = HIGH;
const boolean OFF = LOW;

StatusLED::StatusLED(int red_pin, int green_pin, int blue_pin)
{
    led_pins[0] = red_pin;
    led_pins[1] = green_pin;
    led_pins[2] = blue_pin;
    
    for (int i = 0; i < 3; ++i) {
        pinMode(led_pins[i], OUTPUT);
        led_state[i] = OFF;
    }
    led_flasher = false;
    last_change_time = 0;
    on_period = 500; // milliseconds
}

void StatusLED::SetColour(Colour colour, boolean flash)
{
    switch(colour) {
        case Colour::cINITIALISING:
        case Colour::cNORMAL:
            led_state[0] = OFF; led_state[1] = ON; led_state[2] = OFF; /* Green */
            break;
        case Colour::cCARD_FULL:
            led_state[0] = ON; led_state[1] = ON; led_state[2] = OFF; /* Yellow */
            break;
        case Colour::cALARM:
            led_state[0] = ON; led_state[1] = OFF; led_state[2] = OFF; /* Red */
            break;
    }
    led_flasher = ON;
    if (flash) {
        last_change_time = millis();
    } else {
        last_change_time = 0;
    }
}

void StatusLED::SetStatus(Status status)
{
    switch (status) {
        case Status::sINITIALISING:
            SetColour(Colour::cINITIALISING, true);
            break;
        case Status::sNORMAL:
            SetColour(Colour::cNORMAL);
            break;
        case Status::sCARD_FULL:
            SetColour(Colour::cCARD_FULL, true);
            break;
        case Status::sFATAL_ERROR:
            SetColour(Colour::cALARM, true);
            break;
    }
}

void StatusLED::ProcessFlash(void)
{
    if (last_change_time > 0) {
        // This implies that we are flashing the LED
        if ((last_change_time + on_period) < millis()) {
            last_change_time = millis();
            if (led_flasher == ON) {
                led_flasher = OFF;
            } else {
                led_flasher = ON;
            }
            for (int i = 0; i < 3; ++i) {
                if (led_flasher == ON && led_state[i] == ON)
                    digitalWrite(led_pins[i], ON);
                else
                    digitalWrite(led_pins[i], OFF);
            }
        }
    } else {
        // If we're not flashing, we just set the LED state
        for (int i = 0; i < 3; ++i) {
            digitalWrite(led_pins[i], led_state[i]);
        }
    }
}
