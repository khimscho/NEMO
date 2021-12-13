/*!\file PointBridge.cpp
 * \brief Bridge broadcast UDP from WiFi to NMEA0183
 *
 * This code implements the functionality to capture broadcast UDP packets from the WiFi
 * and transmit them on the first NMEA0183 channel (assuming that it's enabled on the
 * hardware).
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

#include "PointBridge.h"
#include "AsyncUDP.h"
#include "Configuration.h"

namespace nmea {
namespace N0183 {

/// Bring up a UDP to RS-422 packet bridge that captures packets on a known UDP broadcast port via the
/// WiFi interface, and passes them to the first RS-422 serial interface, making the system into a transmitter
/// as well as a receiver.

PointBridge::PointBridge(void)
: m_verbose(false)
{
    m_bridge = new AsyncUDP();
    String port;
    uint16_t port_number;
    logger::LoggerConfig.GetConfigString(logger::Config::ConfigParam::CONFIG_BRIDGE_PORT_S, port);
    if (port.isEmpty()) {
        port_number = 40181;
    } else {
        port_number = port.toInt();
    }
    if (m_bridge->listen(IP_ADDR_BROADCAST, port_number)) {
        Serial.println("INFO: UDP bridge connected.");
        m_bridge->onPacket(std::bind(&PointBridge::HandlePacket, this, std::placeholders::_1));
    }
}

PointBridge::~PointBridge(void)
{
    delete m_bridge;
}

/// Configure the bridge for verbose mode, where it reports all of the packets being transferred to the
/// output RS-422 interface.
///
/// \param state    Flag: True => verbose output, False => quiet output

void PointBridge::SetVerbose(bool state)
{
    m_verbose = state;
}

/// Call-back member function to handle the reception of a packet on the asynchronous UDP thread.  This
/// provides simple logging, and routes the data out through the first RS-422 interface (Serial1)
///
/// \param packet   Packet of UDP information to transfer to output serial

void PointBridge::HandlePacket(AsyncUDPPacket& packet)
{
    Serial1.write(packet.data(), packet.length());
    if (m_verbose) {
        Serial.print("DBG: wrote :");
        Serial.write(packet.data(), packet.length());
        Serial.println("");
    }
}

}
}