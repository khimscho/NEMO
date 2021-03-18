/*! \file eMMCController.cpp
 *  \brief Provide interface to control eMMC module activity.
 *
 * On the NEMO-30 module, the embedded eMMC module has control lines from the ESP32
 * for enable and reset.  This object provides the software abstraction for this.
 *
 * Copyright (c) 2021, University of New Hampshire, Center for Coastal and Ocean Mapping.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <Arduino.h>
#include "eMMCController.h"

namespace nemo30 {

const int enable_pin = 25;
const int reset_pin = 26;
const int reset_delay = 1; // milliseconds, for Toshiba 4GB part (THGBMNG5D1LBAIL)

eMMCController::eMMCController(void)
{
    pinMode(enable_pin, OUTPUT);
    digitalWrite(enable_pin, HIGH);

    pinMode(reset_pin, OUTPUT);
    digitalWrite(reset_pin, HIGH);
}

eMMCController::~eMMCController(void)
{
    digitalWrite(enable_pin, HIGH);
    digitalWrite(reset_pin, HIGH);
}

void eMMCController::setModuleStatus(const bool enabled)
{
    if (enabled) {
        digitalWrite(enable_pin, LOW);
    } else {
        digitalWrite(enable_pin, HIGH);
    }
}

void eMMCController::resetModule(void)
{
    digitalWrite(reset_pin, LOW);
    delay(reset_delay);
    digitalWrite(reset_pin, HIGH);
}

}