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

class NoData(Exception):
    pass

def time_interpolation(filename, elapsed_time_quantum, verbose):
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
    gga_packets = 0  # Count of NMEA0183 GGA packets.
    rmc_packets = 0 # Count of NMEA0183 RMC packets (if GGA is not available)
    dbt_packets = 0  # Count of NMEA0183 DBT packets.
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
                    if isinstance(msg, nmea.RMC):
                        rmc_packets += 1
                    if isinstance(msg, nmea.DBT):
                        dbt_packets += 1
                    if isinstance(msg, nmea.ZDA):
                        zda_packets += 1
                    ascii_packets += 1
                except:
                    if verbose:
                        print("Error parsing NMEA0183 payload \"" + str(pkt.payload) + "\"\n")
                    pass
            packet_count += 1

    if verbose:
        print("Found " + str(packet_count) + " packet total")
        print("Found " + str(systime_packets) + " NMEA2000 SystemTime packets")
        print("Found " + str(depth_packets) + " NMEA2000 Depth packets")
        print("Found " + str(gnss_packets) + " NMEA2000 Position packets")
        print("Found " + str(ascii_packets) + " NMEA0183 packets")
        print("Found " + str(gga_packets) + " NMEA0183 GGA packets")
        print("found " + str(rmc_packets) + " NMEA0183 RMC packets")
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
    hdg_table_t = []
    hdg_table_h = []
    wattemp_table_t = []
    wattemp_table_temp = []
    wind_table_t = []
    wind_table_dir = []
    wind_table_spd = []
    platform_name = "UNKNOWN"
    platform_UUID = "UNKNOWN"

    seconds_per_day = 24.0 * 60.0 * 60.0

    file.seek(0)
    source = LoggerFile.PacketFactory(file)
    elapsed_offset = 0
    last_elapsed = 0
    # Pending packets are used for holding a serial string until the timestamping
    # information is known.
    pending_packet_times = []
    pending_packet_messages = []
    # If the system has no elapsed times (other loggers, sigh), we need to synthesise
    # this, and therefore need to know the real time when "zero elapsed" is assumed
    real_time_elapsed_zero = None
    
    while source.has_more():
        pkt = source.next_packet()
        if pkt is not None:
            # We need to check that the elapsed ms counter hasn't wrapped around since
            # the last packet.  If so, we need to increment the elapsed time base stamp.
            # This method is very simple, and will fail mightily if the elapsed times in
            # the log file are not sequential and monotonic (modulo the elapsed_time_quantum).
            if pkt.elapsed < last_elapsed:
                elapsed_offset = elapsed_offset + elapsed_time_quantum
            last_elapsed = pkt.elapsed
            
            if isinstance(pkt, LoggerFile.Metadata):
                logger_name = pkt.ship_name
                platform_name = pkt.ship_id
            if isinstance(pkt, LoggerFile.SystemTime):
                if use_systime:
                    time_table_t.append(pkt.elapsed + elapsed_offset)
                    time_table_reftime.append(pkt.date * seconds_per_day + pkt.timestamp)
            if isinstance(pkt, LoggerFile.Depth):
                depth_table_t.append(pkt.elapsed + elapsed_offset)
                depth_table_z.append(pkt.depth)
            if isinstance(pkt, LoggerFile.GNSS):
                if use_gnss:
                    time_table_t.append(pkt.elapsed + elapsed_offset)
                    time_table_reftime.append(pkt.date * seconds_per_day + pkt.timestamp)
                position_table_t.append(pkt.elapsed + elapsed_offset)
                position_table_lat.append(pkt.latitude)
                position_table_lon.append(pkt.longitude)
            if isinstance(pkt, LoggerFile.SerialString):
                # In some cases, loggers don't record the elapsed time when the messages were received, so
                # we need to synthesise something to tie everything together.  The conversion script sets the
                # elapsed time in the packet to exactly zero for these types of loggers, so we can use that
                # to test whether we need to do this.
                if pkt.elapsed == 0:
                    no_elapsed_time = True
                else:
                    no_elapsed_time = False
                try:
                    data = pkt.payload.decode('ASCII')
                    if len(data) > 11:
                        try:
                            msg = nmea.parse(data)
                            if isinstance(msg, nmea.ZDA):
                                if use_zda:
                                    if no_elapsed_time:
                                        pkt_real_time = dt.datetime.combine(msg.datestamp, msg.timestamp)
                                        if real_time_elapsed_zero is None:
                                            # First time around: this is our reference time
                                            real_time_elapsed_zero = pkt_real_time
                                            timestamp = 0
                                        else:
                                            timestamp = 1000.0*(pkt_real_time.timestamp() - real_time_elapsed_zero.timestamp())
                                    else:
                                        timestamp = pkt.elapsed + elapsed_offset
                                    reftime = dt.datetime.combine(msg.datestamp, msg.timestamp)
                                    time_table_t.append(timestamp)
                                    time_table_reftime.append(reftime.timestamp())
                                    while len(pending_packet_times) > 0:
                                        packet_time = pending_packet_times.pop()
                                        packet_message = pending_packet_messages.pop()
                                        # We have a pending packet that we have to emit, now we have the timestamp
                                        mean_elapsed_time = (timestamp + packet_time)/2
                                        if isinstance(packet_message, nmea.DBT):
                                            depth = float(packet_message.depth_meters)
                                            depth_table_t.append(mean_elapsed_time)
                                            depth_table_z.append(depth)
                                        elif isinstance(packet_message, nmea.HDT):
                                            heading = float(packet_message.heading)
                                            hdg_table_t.append(mean_elapsed_time)
                                            hdg_table_h.append(heading)
                                        elif isinstance(packet_message, nmea.MWD):
                                            direction = float(packet_message.direction_true)
                                            speed = float(packet_message.wind_speed_meters)
                                            wind_table_t.append(mean_elapsed_time)
                                            wind_table_dir.append(direction)
                                            wind_table_spd.append(speed)
                                        elif isinstance(packet_message, nmea.MTW):
                                            temp = float(packet_message.temperature)
                                            wattemp_table_t.append(mean_elapsed_time)
                                            wattemp_table_temp.append(temp)
                                        else:
                                            longitude = packet_message.longitude
                                            latitude = packet_message.latitude
                                            position_table_t.append(mean_elapsed_time)
                                            position_table_lat.append(latitude)
                                            position_table_lon.append(longitude)
                            if isinstance(msg, nmea.GGA) or isinstance(msg, nmea.RMC):
                                if no_elapsed_time:
                                    # Since we don't have an elapsed time, we need to hold on to this message
                                    # until we get the next time event and can approximate the elapsed timestamp
                                    if len(time_table_t) > 0:
                                        pending_packet_times.append(time_table_t[-1])
                                        pending_packet_messages.append(msg)
                                else:
                                    # Convert all of the elements first to make sure we have valid conversion
                                    timestamp = pkt.elapsed + elapsed_offset
                                    latitude = msg.latitude
                                    longitude = msg.longitude
                                    # Add all of the elements as a group
                                    position_table_t.append(timestamp)
                                    position_table_lat.append(latitude)
                                    position_table_lon.append(longitude)
                            if isinstance(msg, nmea.DBT):
                                if msg.depth_meters is None:
                                    print('Undefined depth: ' + str(msg))
                                else:
                                    if no_elapsed_time:
                                        if len(time_table_t) > 0:
                                            pending_packet_times.append(time_table_t[-1])
                                            pending_packet_messages.append(msg)
                                    else:
                                        timestamp = pkt.elapsed + elapsed_offset
                                        depth = float(msg.depth_meters)
                                        depth_table_t.append(timestamp)
                                        depth_table_z.append(depth)
                            if isinstance(msg, nmea.HDT):
                                if no_elapsed_time:
                                    if len(time_table_t) > 0:
                                        pending_packet_times.append(time_table_t[-1])
                                        pending_packet_messages.append(msg)
                                else:
                                    timestamp = pkt.elapsed + elapsed_offset
                                    heading = float(msg.heading)
                                    hdg_table_t.append(timestamp)
                                    hdg_table_h.append(heading)
                            if isinstance(msg, nmea.MWD):
                                if no_elapsed_time:
                                    if len(time_table_t) > 0:
                                        pending_packet_times.append(time_table_t[-1])
                                        pending_packet_messages.append(msg)
                                else:
                                    timestamp = pkt.elapsed + elapsed_offset
                                    direction = float(msg.direction_true)
                                    speed = float(msg.wind_speed_meters)
                                    wind_table_t.append(timestamp)
                                    wind_table_dir.append(direction)
                                    wind_table_spd.append(speed)
                            if isinstance(msg, nmea.MTW):
                                if no_elapsed_time:
                                    if len(time_table_t) > 0:
                                        pending_packet_times.append(time_table_t[-1])
                                        pending_packet_messages.append(msg)
                                else:
                                    timestamp = pkt.elapsed + elapsed_offset
                                    temp = float(msg.temperature)
                                    wattemp_table_t.append(timestamp)
                                    wattemp_table_temp.append(temp)
                        except nmea.ParseError as e:
                            if verbose:
                                print('Parse error: {}'.format(e))
                            continue
                        except AttributeError as e:
                            print('Attribute error: {}'.format(e))
                            continue
                        except ChecksumError as e:
                            print('Checksum error: {}'.format(e))
                            continue
                    else:
                        # Packets have to be at least 11 characters to contain all of the mandatory elements.
                        # Usually a short packet is broken in some fashion, and should be ignored.
                        print('Ignoring short message: ' + str(data)) 
                except UnicodeDecodeError:
                    if verbose:
                        print('Decode error: {}'.format(e))
                    continue
    if verbose:
        print('Reference time table length = ', len(time_table_t))
        print('Position table length = ', len(position_table_t))
        print('Depth observations = ', len(depth_table_t))

    if len(depth_table_t) < 1:
        raise NoData()
        
    # Finally, do the interpolations to generate the output data
    z_times = np.interp(depth_table_t, time_table_t, time_table_reftime)
    z_lat = np.interp(depth_table_t, position_table_t, position_table_lat)
    z_lon = np.interp(depth_table_t, position_table_t, position_table_lon)
    
    hdg_times = np.interp(hdg_table_t, time_table_t, time_table_reftime)
    hdg_lat = np.interp(hdg_table_t, position_table_t, position_table_lat)
    hdg_lon = np.interp(hdg_table_t, position_table_t, position_table_lon)
    
    wt_times = np.interp(wattemp_table_t, time_table_t, time_table_reftime)
    wt_lat = np.interp(wattemp_table_t, position_table_t, position_table_lat)
    wt_lon = np.interp(wattemp_table_t, position_table_t, position_table_lon)
    
    wind_times = np.interp(wind_table_t, time_table_t, time_table_reftime)
    wind_lat = np.interp(wind_table_t, position_table_t, position_table_lat)
    wind_lon = np.interp(wind_table_t, position_table_t, position_table_lon)

    return {
        'loggername': logger_name,
        'platform': platform_name,
        'depth' : {
            't': z_times,
            'lat': z_lat,
            'lon': z_lon,
            'z': depth_table_z
        },
        'heading' : {
            't': hdg_times,
            'lat': hdg_lat,
            'lon': hdg_lon,
            'heading': hdg_table_h
        },
        'watertemp' : {
            't': wt_times,
            'lat': wt_lat,
            'lon': wt_lon,
            'temperature': wattemp_table_temp
        },
        'wind' : {
            't': wind_times,
            'lat': wind_lat,
            'lon': wind_lon,
            'direction': wind_table_dir,
            'speed': wind_table_spd
        }
    }
