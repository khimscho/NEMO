/*! \file YDVRSource.cpp
 *  \brief Implementation of PacketSource for the Yacht Devices YDVR DAT format.
 *
 *  This provides the concrete implementation of the PacketSource abstract interface
 *  specialised for the Yacht Devices YDVR04's DAT data format (encoded NMEA2000 packets
 *  with internal timestamp and PGN number).
 *
 * Copyright (c) 2021, University of New Hampshire, Center for Coastal and Ocean Mapping and
 * NOAA/UNH Joint Hydrographic Center.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <stdint.h>
#include <set>
#include "YDVRSource.h"
#include "NMEA2000.h"

const std::set<uint32_t> multi_packet({
     65240, 126208, 126464, 126720, 126983, 126984, 126985, 126986, 126987, 126988, 126996,
    126998, 127233, 127237, 127489, 127496, 127497, 127498, 127503, 127504, 127506, 127507,
    127509, 127510, 127511, 127512, 127513, 127514, 128275, 128520, 129029, 129038, 129039,
    129040, 129041, 129044, 129045, 129284, 129285, 129301, 129302, 129538, 129540, 129541,
    129542, 129545, 129547, 129549, 129551, 129556, 129792, 129793, 129794, 129795, 129796,
    129797, 129798, 129799, 129800, 129801, 129802, 129803, 129804, 129805, 129806, 129807,
    129808, 129809, 129810, 130052, 130053, 130054, 130060, 130061, 130064, 130065, 130066,
    130067, 130068, 130069, 130070, 130071, 130072, 130073, 130074, 130320, 130321, 130322,
    130323, 130324, 130567, 130577, 130578, 130816 });

YDVRSource::YDVRSource(FILE *f)
: PacketSource(), m_source(f)
{
}

YDVRSource::~YDVRSource(void)
{
}

void CanIdToN2k(unsigned long id, unsigned char &prio, unsigned long &pgn, unsigned char &src, unsigned char &dst) {
  unsigned char CanIdPF = (unsigned char) (id >> 16);
  unsigned char CanIdPS = (unsigned char) (id >> 8);
  unsigned char CanIdDP = (unsigned char) (id >> 24) & 1;

    src = (unsigned char) id >> 0;
    prio = (unsigned char) ((id >> 26) & 0x7);

    if (CanIdPF < 240) {
      /* PDU1 format, the PS contains the destination address */
        dst = CanIdPS;
        pgn = (((unsigned long)CanIdDP) << 16) | (((unsigned long)CanIdPF) << 8);
    } else {
      /* PDU2 format, the destination is implied global and the PGN is extended */
        dst = 0xff;
        pgn = (((unsigned long)CanIdDP) << 16) | (((unsigned long)CanIdPF) << 8) | (unsigned long)CanIdPS;
    }
}

bool YDVRSource::NextPacket(tN2kMsg& msg)
{
    uint16_t timestamp;
    uint32_t msgID;
    uint8_t buffer[1024];
    
    if (fread(&timestamp, sizeof(uint16_t), 1, m_source) != 1) return false;
    if (fread(&msgID, sizeof(uint32_t), 1, m_source) != 1) return false;
    
    unsigned char priority, source, destination;
    unsigned long pgn;

    if (msgID == 0xFFFFFFFF) {
        // Control packet
        priority = source = destination = 0;
        pgn = msgID;
    } else {
        // We need to decode the message ID in order to get PGN, source, destination, etc.
        CanIdToN2k(msgID, priority, pgn, source, destination);
    }
    msg.PGN = pgn;
    msg.MsgTime = timestamp;
    msg.Source = source;
    msg.Destination = destination;

    if (pgn == 59904) {
        /* We should expect three bytes in file */
        msg.DataLen = 3;
    } else if (pgn == 0xFFFFFFFF) {
        /* This is a control packet for the logger, of eight bytes */
        msg.DataLen = 8;
    } else if (IsMultiPacket(pgn)) {
        /* Multi-packet packets have an embedded length */
        fread(buffer, sizeof(uint8_t), 2, m_source);
        msg.DataLen = buffer[1];
    } else {
        /* Unless it's one of the specific packets, all packets are eight bytes */
        msg.DataLen = 8;
    }
    
    if (msg.DataLen >= msg.MaxDataLen) {
        throw DataPacketTooLarge();
    }
    
    if (fread(msg.Data, sizeof(uint8_t), msg.DataLen, m_source) != msg.DataLen) return false;
    
    return true;
}

bool YDVRSource::IsMultiPacket(uint32_t pgn)
{
    // Unfortunately, there isn't a decent way to check for which packets are multi-message
    // except to exhaustively check the list ...
    if (multi_packet.find(pgn) == multi_packet.end())
        return false;
    else
        return true;
}
