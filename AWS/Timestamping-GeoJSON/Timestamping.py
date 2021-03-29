# \file timestamp_data.py
# \brief Read a binary data file from the SB2030 data logger, and generate timestamped data
#
# The SB2030 data logger reports, for each packet received, the elapsed time with respect
# to the logger's time ticker, and the data.  It also estimates the real time for the packet's
# reception time using whatever time information it has available, in real-time, using a forward
# extrapolation of the last time reference.  Of course, this isn't great, because it has to be
# causal.  This code is designed to get around that by reading all of the data to find the best
# time reference information, and then going back through again to provide timestamps for the
# packets based on all of the time reference data.
#    The code is pretty simple, just doing linear interpolation between timestamps of the reference
# time in order to generate the timestamps.  Reference time is, by preference NMEA2000 SystemTime,
# then the timestamps from NMEA2000 GNSS packets, and then finally from NMEA0183 GPGGA packets if
# there's nothing else available.
#
# Copyright 2020 Center for Coastal and Ocean Mapping & NOAA-UNH Joint
# Hydrographic Center, University of New Hampshire.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights to use,
# copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
# OR OTHER DEALINGS IN THE SOFTWARE.

import numpy as np
import pynmea2 as nmea
import LoggerFile
import datetime as dt

class NoTimeSource(Exception):
    pass


def time_interpolation(filename):
    # Assume that we're dealing with a file, rather than a stream, which is provided as the first
    # argument on the command line; it's a requirement that this is also rewind-able, so that we
    # can take a second pass through the file to determine the timestamps.
    file = open(filename, 'rb')

    # First pass through the file: we need to count the packets in order to determine what sources
    # of information we have, and should use for timestamping and positioning.

    packet_count = 0  # Total number of packets of all types
    systime_packets = 0  # Count of NMEA2000 SystemTime packets
    depth_packets = 0  # Count of NMEA2000 Depth packets
    gnss_packets = 0  # Count of NMEA2000 GNSS packets
    ascii_packets = 0  # Count of all NMEA0183 packets (which all show up as the same type to start with)
    gga_packets = 0  # Count of NMEA0183 GPGGA packets.
    dbt_packets = 0  # Count of NMEA0183 SDDBT packets.
    zda_packets = 0  # Count of NMEA0183 ZDA packets.

    source = LoggerFile.PacketFactory(file)

    while source.has_more():
        pkt = source.next_packet()
        if pkt is not None:
            if isinstance(pkt, LoggerFile.SystemTime):
                systime_packets += 1
            if isinstance(pkt, LoggerFile.Depth):
                depth_packets += 1
            if isinstance(pkt, LoggerFile.GNSS):
                gnss_packets += 1
            if isinstance(pkt, LoggerFile.SerialString):
                # Note that parsing the NMEA string doesn't guarantee that it's actually valid,
                # and you can end up with problems later on when getting at the data.
                try:
                    msg = nmea.parse(pkt.payload.decode('ASCII'))
                    if isinstance(msg, nmea.GGA):
                        gga_packets += 1
                    if isinstance(msg, nmea.DBT):
                        dbt_packets += 1
                    if isinstance(msg, nmea.ZDA):
                        zda_packets += 1
                    ascii_packets += 1
                except:
                    print("Error parsing NMEA0183 payload \"" + str(pkt.payload) + "\"\n")
                    pass
            packet_count += 1

    print("Found " + str(packet_count) + " packet total")
    print("Found " + str(systime_packets) + " NMEA2000 SystemTime packets")
    print("Found " + str(depth_packets) + " NMEA2000 Depth packets")
    print("Found " + str(gnss_packets) + " NMEA2000 Position packets")
    print("Found " + str(ascii_packets) + " NMEA0183 packets")
    print("Found " + str(gga_packets) + " NMEA0183 GGA packets")
    print("Found " + str(dbt_packets) + " NMEA0183 DBT packets")
    print("Found " + str(zda_packets) + " NMEA0183 ZDA packets")

    # We need to decide on the source of master time provision.  In general, we prefer to use NMEA2000
    # SystemTime if it's available, but otherwise use either GNSS information for NMEA2000, or ZDA
    # information for NMEA0183, preferring GNSS if both are available.  The ordering reflects the idea
    # that NMEA2000 should have lower (or at least better controlled) latency than NMEA0183 due to not
    # being on a 4800-baud serial line.

    use_systime = False
    use_zda = False
    use_gnss = False
    if systime_packets > 0:
        use_systime = True
    else:
        if gnss_packets > 0:
            use_gnss = True
        else:
            if zda_packets > 0:
                use_zda = True
            else:
                raise NoTimeSource()

    # Second pass through the file, converting into the lists needed for interpolation, using the time source
    # determined above.

    time_table_t = []
    time_table_reftime = []
    position_table_t = []
    position_table_lon = []
    position_table_lat = []
    depth_table_t = []
    depth_table_z = []
    platform_name = "UNKNOWN"
    platform_UUID = "UNKNOWN"

    seconds_per_day = 24.0 * 60.0 * 60.0

    file.seek(0)
    source = LoggerFile.PacketFactory(file)
    while source.has_more():
        pkt = source.next_packet()
        if pkt is not None:
            if isinstance(pkt, LoggerFile.Metadata):
                platform_name = pkt.ship_name
                platform_UUID = pkt.ship_id
            if isinstance(pkt, LoggerFile.SystemTime):
                if use_systime:
                    time_table_t.append(pkt.elapsed)
                    time_table_reftime.append(pkt.date * seconds_per_day + pkt.timestamp)
            if isinstance(pkt, LoggerFile.Depth):
                depth_table_t.append(pkt.elapsed)
                depth_table_z.append(pkt.depth)
            if isinstance(pkt, LoggerFile.GNSS):
                if use_gnss:
                    time_table_t.append(pkt.elapsed)
                    time_table_reftime.append(pkt.date * seconds_per_day + pkt.timestamp)
                position_table_t.append(pkt.elapsed)
                position_table_lat.append(pkt.latitude)
                position_table_lon.append(pkt.longitude)
            if isinstance(pkt, LoggerFile.SerialString):
                try:
                    msg = nmea.parse(pkt.payload.decode('ASCII'))
                    if isinstance(msg, nmea.ZDA):
                        if use_zda:
                            timestamp = pkt.elapsed
                            reftime = dt.datetime.combine(msg.datestamp, msg.timestamp)
                            time_table_t.append(timestamp)
                            time_table_reftime.append(reftime.timestamp())
                    if isinstance(msg, nmea.GGA):
                        # Convert all of the elements first to make sure we have valid conversion
                        timestamp = pkt.elapsed
                        latitude = msg.latitude
                        longitude = msg.longitude
                        # Add all of the elements as a group
                        position_table_t.append(timestamp)
                        position_table_lat.append(latitude)
                        position_table_lon.append(longitude)
                    if isinstance(msg, nmea.DBT):
                        timestamp = pkt.elapsed
                        depth = float(msg.depth_meters)
                        depth_table_t.append(timestamp)
                        depth_table_z.append(depth)
                except nmea.ParseError as e:
                    print('Parse error: {}'.format(e))
                    continue
                except AttributeError as e:
                    print('Attribute error: {}'.format(e))
                    continue
                except ChecksumError as e:
                    print('Checksum error: {}'.format(e))
                    continue

    print('Reference time table length = ', len(time_table_t))
    print('Position table length = ', len(position_table_t))
    print('Depth observations = ', len(depth_table_t))

    # Finally, do the interpolations to generate the output data
    ref_times = np.interp(depth_table_t, time_table_t, time_table_reftime)
    ref_lat = np.interp(depth_table_t, position_table_t, position_table_lat)
    ref_lon = np.interp(depth_table_t, position_table_t, position_table_lon)

    return {
        'name': platform_name,
        'uniqid': platform_UUID,
        't': ref_times,
        'lat': ref_lat,
        'lon': ref_lon,
        'z': depth_table_z
    }
