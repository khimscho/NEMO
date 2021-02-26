/*!\file SupplyMonitor.cpp
 * \brief Interface for NEMO-30 power monitoring
 *
 * The NEMO-30 variant of the logger has the ability to monitor the input power feed
 * so that it can detect when the emergency backup power has kicked in, and therefore
 * turn off the logging for a safe shutdown before it gives out.
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
#include "SupplyMonitor.h"
#include "Configuration.h"

namespace nemo30 {

SupplyMonitor::SupplyMonitor(uint8_t monitor_pin)
: m_monitorPower(false), m_monitorPin(monitor_pin)
{
    if (!logger::LoggerConfig.GetConfigBinary(logger::Config::ConfigParam::CONFIG_POWMON_B, m_monitorPower)) {
        // Returns false if the key doesn't exist.  This usually means that
        // it hasn't been set yet, and that means we're still in programming,
        // or debugging mode.  In that mode, the power monitoring doesn't work
        // (there's no input power to monitor!), so we turn monitoring off.
        m_monitorPower = false;
    }
    if (m_monitorPower) {
        pinMode(m_monitorPin, INPUT);
    }
}

bool SupplyMonitor::EmergencyPower(void)
{
    if (m_monitorPower) {
        uint16_t monitor_voltage = analogRead(m_monitorPin);
        if (monitor_voltage < 2048)
            return true;
    }
    return false;
}

}