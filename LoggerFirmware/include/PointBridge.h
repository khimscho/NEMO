/*! \file PointBridge.h
 *  \brief Simple UDP broadcast bridge to NMEA0183
 *
 * Under exceptional circumstances, it might be interesting to capture UDP traffic of NMEA
 * strings on the WiFi, given a broadcast port, and send them to the NMEA0183 hardware
 * outputs (if available and enabled).  In this implementation, anything sent to the
 * configured port is echoed to the first NMEA0183 channel without checking --- the messages
 * must therefore be good as is!
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

#ifndef __POINT_BRIDGE_H__
#define __POINT_BRIDGE_H__

#include <stdint.h>
#include "Arduino.h"
#include "AsyncUDP.h"

namespace nmea {
namespace N0183 {

/// \class PointBridge
/// \brief Use the asynchronous UDP facilities to capture broadcast packets in the background

class PointBridge {
public:
    PointBridge(void);
    ~PointBridge(void);

    void SetVerbose(bool state);
private:
    AsyncUDP    *m_bridge;
    bool        m_verbose;

    void HandlePacket(AsyncUDPPacket& packet);
};

}
}

#endif
