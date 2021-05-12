## @file LoggerFile.py
# @brief Library objects for reading Seabed 2030 data logger files
#
# The Seabed 2030 low-cost logger generates files from NMEA2000 onto the SD card in
# fairly efficient binary format, with a timestamp from the local machine.  The code
# here unpacks this format and makes the data available.
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

import struct

## Convert from Kelvin to degrees Celsius
#
# Temperature is stored in the NMEA2000 packets as Kelvin, but that isn't terribly useful for end users.  This converts
# into degrees Celsius so that output is more useable.
#
# \param temp   Temperature in Kelvin
# \return Temperature in degrees Celsius
def temp_to_celsius(temp):
    return temp - 273.15

## Convert from Pascals to millibars
#
# Pressure is stored in the NMEA2000 packets as Pascals, but that isn't terribly useful for end users.  This converts
# into millibars so that output is more useable.
#
# \param pressure   Pressure in Pascals
# \return Pressure in millibars
def pressure_to_mbar(pressure):
    return pressure / 100.0

## Convert from radians to degrees
#
# Angles are stored in the NMEA2000 packets as radians, but that isn't terribly useful for end users (at least for
# display).  This converts into degrees so that output is more useable.
#
# \param rads   Angle in radians
# \return Angle in degrees
def angle_to_degs(rads):
    return rads*180.0/3.1415926535897932384626433832795

## Base class for all data packets that can be read from the binary file
#
# This provides a common base class for all of the data packets, and stores the information on the date and time at
# which the packet was received.
class DataPacket:
    ## Initialise the base packet with date and timestamp for the packet reception time
    #
    # This simply stores the date and time for the packet reception
    #
    # \param self       Pointer to the object
    # \param date       Days elapsed since 1970-01-01
    # \param timestamp  Seconds since midnight on the day
    def __init__(self, date, timestamp, elapsed):
        ## Date in days since 1970-01-01
        self.date = date
        ## Time in seconds since midnight on the day in question
        self.timestamp = timestamp
        ## Time in milliseconds since boot (reference time)
        self.elapsed = elapsed

    ## Implement the printable interface for this class, allowing it to be streamed
    #
    # This converts to human-readable version of the data packet for the standard streaming output interface.
    #
    # \param self   Pointer to the object
    # \return String representation of the object
    def __str__(self):
        rtn = "[" + str(self.date) + " days, " + str(self.timestamp) + " s., " + str(self.elapsed) + " ms elapsed]"
        return rtn

## Implementation of the SystemTime NMEA2000 packet
#
# This retrieves the timestamp, logger elapsed time, and time source for a SystemTime packet serialised into the file.
#
class SystemTime(DataPacket):
    ## Initialise the SystemTime packet with date/time of reception, and logger elapsed time
    #
    # This picks out the date and timestamp for the packet (which is the indicated real time in the packet itself), and
    # then the logger elapsed time and data source (u16, double, u32, u8), total 15B.
    #
    # \param self   Pointer to the object
    # \param buffer Binary data from file to interpret
    def __init__(self, buffer):
        (date, timestamp, elapsed_time, data_source) = struct.unpack("<HdIB", buffer)
        ## Source of the timestamp (see documentation for decoding, but at least GNSS)
        self.data_source = data_source
        DataPacket.__init__(self, date, timestamp, elapsed_time)

    ## Provide the fixed-text string name for this data packet
    #
    # This simply reports the human-readable name for the class so that reporting is possible
    #
    # \param self   Pointer to the object
    # \return String with the human-readable name of the packet
    def name(self):
        return "SystemTime"

    ## Implement the printable interface for this class, allowing it to be streamed
    #
    # This converts to human-readable version of the data packet for the standard streaming output interface.
    #
    # \param self   Pointer to the object
    # \return String representation of the object
    def __str__(self):
        rtn = DataPacket.__str__(self) + " " + self.name() + ":  source = " + str(self.data_source)
        return rtn

## Implementation of the Attitude NMEA2000 packet
#
# The attitude message contains estimates of roll, pitch, and yaw of the ship, without any indication of where the data
# is coming from.  Consequently, the data is just reported directly.
class Attitude(DataPacket):
    ## Initialise the Attitude packet with reception timestamp, and raw data
    #
    # This picks out the date and time of message reception (based on the last known good real time estimate), and the
    # raw attitude data (u16, double, double, double, double), total 34B.  Attitude values are in radians.
    #
    # \param self   Pointer to the object
    # \param buffer Binary data from file to interpret.
    def __init__(self, buffer):
        (date, timestamp, elapsed_time, yaw, pitch, roll) = struct.unpack("<HdIddd", buffer)
        ## Yaw angle of the ship, radians (+ve clockwise from north)
        self.yaw = yaw
        ## Pitch angle of the ship, radians (+ve bow up)
        self.pitch = pitch
        ## Roll angle of the ship, radians (+ve port up)
        self.roll = roll
        DataPacket.__init__(self, date, timestamp, elapsed_time)

    ## Provide the fixed-text string name for this data packet
    #
    # This simply reports the human-readable name for the class so that reporting is possible
    #
    # \param self   Pointer to the object
    # \return String with the human-readable name of the packet
    def name(self):
        return "Attitude"

    ## Implement the printable interface for this class, allowing it to be streamed
    #
    # This converts to human-readable version of the data packet for the standard streaming output interface.
    #
    # \param self   Pointer to the object
    # \return String representation of the object
    def __str__(self):
        rtn = DataPacket.__str__(self) + " " + self.name() + ": yaw = " + str(angle_to_degs(self.yaw))\
              + " deg, pitch = " + str(angle_to_degs(self.pitch))\
              + " deg, roll = " + str(angle_to_degs(self.roll)) + " deg"
        return rtn

## Implement the Observed Depth NMEA2000 message
#
# The depth message includes the observed depth, the offset that needs to be applied to it either for rise from the keel
# or waterline, and the maximum depth that can be observed (allowing for some filtering).
class Depth(DataPacket):
    ## Initialise the Depth packet with reception timestamp and raw data
    #
    # This picks out the date and time of message reception (based on the last known good real time estimate), and the
    # raw depth, offset, and range data (u16, double, double, double, double) for 34B total.  Depths are in metres
    #
    # \param self   Pointer to the object
    # \param buffer Binary data from file to interpret
    def __init__(self, buffer):
        (date, timestamp, elapsed_time, depth, offset, range) = struct.unpack("<HdIddd", buffer)
        ## Observed depth below transducer, metres
        self.depth = depth
        ## Offset for depth, metres.
        # This is an offset to apply to reference the depth to either the water surface, or the keel.  Positive
        # values imply that the correction is for water surface to transducer; negative implies transducer to keel
        self.offset = offset
        ## Maximum range of observation, metres
        self.range = range
        DataPacket.__init__(self, date, timestamp, elapsed_time)

    ## Provide the fixed-text string name for this data packet
    #
    # This simply reports the human-readable name for the class so that reporting is possible
    #
    # \param self   Pointer to the object
    # \return String with the human-readable name of the packet
    def name(self):
        return "Depth"

    ## Implement the printable interface for this class, allowing it to be streamed
    #
    # This converts to human-readable version of the data packet for the standard streaming output interface.
    #
    # \param self   Pointer to the object
    # \return String representation of the object
    def __str__(self):
        rtn = DataPacket.__str__(self) + " " + self.name() + ": depth = " + str(self.depth) + "m, offset = "\
              + str(self.offset) + "m, range = " + str(self.range) + "m"
        return rtn

## Implement the Course-over-Ground Rapid NMEA2000 message
#
# The Course-over-ground/Speed-over-ground message is sent more frequently that most, and contains estimates of the
# current course and speed.
class COG(DataPacket):
    ## Initialise the COG-SOG packet with reception timestamp and raw data
    #
    # This picks out the date and time of message reception (based on the last known good real time estimate), and the
    # course/speed over ground (u16, double, double, double) for 26B total.  Course is in radians, speed in m/s.
    #
    # \param self   Pointer to the objet
    # \param buffer Binary data from file to interpret
    def __init__(self, buffer):
        (date, timestamp, elapsed_time, courseOverGround, speedOverGround) = struct.unpack("<HdIdd", buffer)
        ## Course over ground (radians)
        self.courseOverGround = courseOverGround
        ## Speed over ground (m/s)
        self.speedOverGround = speedOverGround
        DataPacket.__init__(self, date, timestamp, elapsed_time)

    ## Provide the fixed-text string name for this data packet
    #
    # This simply reports the human-readable name for the class so that reporting is possible
    #
    # \param self   Pointer to the object
    # \return String with the human-readable name of the packet
    def name(self):
        return "Course Over Ground"

    ## Implement the printable interface for this class, allowing it to be streamed
    #
    # This converts to human-readable version of the data packet for the standard streaming output interface.
    #
    # \param self   Pointer to the object
    # \return String representation of the object
    def __str__(self):
        rtn = DataPacket.__str__(self) + " " + self.name() + ": course over ground = "\
              + str(angle_to_degs(self.courseOverGround)) + " deg, speed over ground = "\
              + str(self.speedOverGround) + " m/s"
        return rtn

## Implement the GNSS observation NMEA2000 message
#
# The GNSS observation message contains a single GNSS observation from a receiver on the ship (multiple receivers are
# possible, of course).  This contains all of the usual suspects that would come from a GPGGA message in NMEA0183, but
# has better information on correctors, and methods of correction, which are preserved here.
class GNSS(DataPacket):
    ## Initialise the GNSS packet with real time timestamp and raw data
    #
    # This picks out the raw data, including the validity time of the original message, including the latitude,
    # longitude, altitude, receiver type, receiver method, number of SVs, horizontal DOP, position DOP, geoid sep.,
    # number of reference stations, reference station type, reference station ID, and correction age, as
    # (u16, double, double, double, double, u8, u8, u8, double, double, double, u8, u8, u16, double) for
    # 73B total.  Latitude, longitude are in degrees; altitude, separation are metres; others are integers that are
    # mapped enum values for the receiver type (GPS, GLONASS, Galileo, etc.) and so on.  See Wiki for definitions.
    #
    # \param self   Pointer to the object
    # \param buffer Binary data from file to interpret
    def __init__(self, buffer):
        (sys_date, sys_timestamp, sys_elapsed, date, timestamp, latitude, longitude, altitude,
         receiverType, receiverMethod, numSVs, horizontalDOP, positionDOP, separation, numRefStations, refStationType,
         refStationID, correctionAge) = struct.unpack("<HdIHddddBBBdddBBHd", buffer)
        ## In-message date (days since epoch)
        self.date = date
        ## In-message timestamp (seconds since midnight)
        self.timestamp = timestamp
        ## Latitude of position, degrees
        self.latitude = latitude
        ## Longitude of position, degrees
        self.longitude = longitude
        ## Altitude of position, metres
        self.altitude = altitude
        ## GNSS receiver type (e.g., GPS, GLONASS, Beidou, Galileo, and some combinations)
        self.receiverType = receiverType
        ## GNSS receiver method (e.g., C/A, Differential, Float/fixed RTK, etc.)
        self.receiverMethod = receiverMethod
        ## Number of SVs in view
        self.numSVs = numSVs
        ## Horizontal dilution of precision (unitless)
        self.horizontalDOP = horizontalDOP
        ## Position dilution of precision (unitless)
        self.positionDOP = positionDOP
        ## Geoid-ellipsoid separation, metres (modeled)
        self.separation = separation
        ## Number of reference stations used in corrections
        self.numRefStations = numRefStations
        ## Reference station receiver type (as for receiverType)
        self.refStationType = refStationType
        ## Reference station ID number
        self.refStationID = refStationID
        ## Age of corrections, seconds
        self.correctionAge = correctionAge
        DataPacket.__init__(self, sys_date, sys_timestamp, sys_elapsed)

    ## Provide the fixed-text string name for this data packet
    #
    # This simply reports the human-readable name for the class so that reporting is possible
    #
    # \param self   Pointer to the object
    # \return String with the human-readable name of the packet
    def name(self):
        return "GNSS"

    ## Implement the printable interface for this class, allowing it to be streamed
    #
    # This converts to human-readable version of the data packet for the standard streaming output interface.
    #
    # \param self   Pointer to the object
    # \return String representation of the object
    def __str__(self):
        rtn = DataPacket.__str__(self) + " " + self.name() + ": in-message date = " + str(self.date) + " days, "\
              + "in-message time = " + str(self.timestamp) + " s.,  latitude = " + str(self.latitude)\
              + " deg, longitude = " + str(self.longitude) + " deg, altitude = " + str(self.altitude)\
              + " m, GNSS type = " + str(self.receiverType) + ", GNSS method = " + str(self.receiverMethod)\
              + ", num. SVs = " + str(self.numSVs) + ", horizontal DOP = " + str(self.horizontalDOP)\
              + ", position DOP = " + str(self.positionDOP) + ", Geoid separation = " + str(self.separation)\
              + "m, number of ref. stations = " + str(self.numRefStations)\
              + ", ref. station type = " + str(self.refStationType) + ", ref. station ID = " + str(self.refStationID)\
              + ", correction age = " + str(self.correctionAge)
        return rtn

## Implement the Environment NMEA2000 message
#
# The Environment message was originally used to provide a combination of temperature, humidity, and pressure, but has
# since been deprecated in favour of individual messages (which also have the benefit of preserving the source information
# for the pressure data).  These are also supported, but this is provided for backwards compatibility.
class Environment(DataPacket):
    ## Initialise the Environment packet
    #
    # This picks out the date and time of message reception (based on the last known good real time estimate), and the
    # temperature (and source), humidity (and source), and pressure information as (u16, double, u8, double, u8, double,
    # double) for 36B total.  Temperature is Kelvin, humidity is %, pressure is Pascals.  The temperature and humidity
    # sources are enums (see Wiki for details); some filtering on allowed sources can happen at the logger, so not all
    # data might make it here from the NMEA2000 bus.
    #
    # \param self   Pointer to the object
    # \param buffer Binary data from file to interpret
    def __init__(self, buffer):
        (date, timestamp, elapsed_time, tempSource, temperature, humiditySource, humidity, pressure) = \
            struct.unpack("<HdIBdBdd", buffer)
        ## Source of temperature information (e.g., inside, outside)
        self.tempSource = tempSource
        ## Current temperature, Kelvin
        self.temperature = temperature
        ## Source of humidity information (e.g., inside, outside)
        self.humiditySource = humiditySource
        ## Relative humidity, percent
        self.humidity = humidity
        ## Current pressure, Pascals.
        # The source information for pressure information is not provided, so presumably this is meant to be
        # atmospheric pressure, rather than something more general.
        self.pressure = pressure
        DataPacket.__init__(self, date, timestamp, elapsed_time)

    ## Provide the fixed-text string name for this data packet
    #
    # This simply reports the human-readable name for the class so that reporting is possible
    #
    # \param self   Pointer to the object
    # \return String with the human-readable name of the packet
    def name(self):
        return "Environment"

    ## Implement the printable interface for this class, allowing it to be streamed
    #
    # This converts to human-readable version of the data packet for the standard streaming output interface.
    #
    # \param self   Pointer to the object
    # \return String representation of the object
    def __str__(self):
        rtn = DataPacket.__str__(self) + " " + self.name() + ": temperature = " + str(temp_to_celsius(self.temperature)) + " ºC (source "\
              + str(self.tempSource) + "), humidity = " + str(self.humidity) + "% (source " + str(self.humiditySource) +\
              "), pressure = " + str(pressure_to_mbar(self.pressure)) + " mBar"
        return rtn

## Implement the Temperature NMEA2000 message
#
# The Temperature message can serve a number of purposes, carrying temperature information for a variety of different
# sensors on the ship, including things like bait tanks and reefers.  The information is, however, always qualified
# with a source designator.  Some filtering of messages might happen at the logger, however, which means that not all
# temperature messages make it to here.
class Temperature(DataPacket):
    ## Initialise the Temperature message
    #
    # This picks out the date and time of message reception (based on the last known good real time estimate), and the
    # temperature source and temperature as (u16, double, u8, double) for 19B total.  Temperature source is a mapped
    # enum (see Wiki for details); temperature is Kelvin, so it's always positive.
    #
    # \param self   Pointer to the object
    # \param buffer Binary data from file to interpret
    def __init__(self, buffer):
        (date, timestamp, elapsed_time, tempSource, temperature) = struct.unpack("<HdIBd", buffer)
        ## Source of temperature information (e.g., water, air, cabin)
        self.tempSource = tempSource
        ## Temperature of source, Kelvin
        self.temperature = temperature
        DataPacket.__init__(self, date, timestamp, elapsed_time)

    ## Provide the fixed-text string name for this data packet
    #
    # This simply reports the human-readable name for the class so that reporting is possible
    #
    # \param self   Pointer to the object
    # \return String with the human-readable name of the packet
    def name(self):
        return "Temperature"

    ## Implement the printable interface for this class, allowing it to be streamed
    #
    # This converts to human-readable version of the data packet for the standard streaming output interface.
    #
    # \param self   Pointer to the object
    # \return String representation of the object
    def __str__(self):
        rtn = DataPacket.__str__(self) + " " + self.name() + ": temperature = " + str(temp_to_celsius(self.temperature))\
              + " ºC (source " + str(self.tempSource) + ")"
        return rtn

## Implement the Humidity NMEA2000 message
#
# The Humidity message can serve a number of purposes, carrying humidity information for a variety of different sensors
# on the ship, including interior and exterior.  The information is, however, always qualified with a source designator.
# Some filtering of messages might happen at the logger, however, which means that not all humidity messages make it
# to here.
class Humidity(DataPacket):
    ## Initialise the Humidty message
    #
    # This picks out the date and time of message reception (based on the last known good real time estimate), and the
    # humidity source and humidity as (u16, double, u8, double) for 19B total.  Humidity source is a mapped enum (see
    # Wiki for details); humidity is a relative percentage.
    #
    # \param self   Pointer to the object
    # \param buffer Binary data from file to interpret
    def __init__(self, buffer):
        (date, timestamp, elapsed_time, humiditySource, humidity) = struct.unpack("<HdIBd", buffer)
        ## Source of humidity (e.g., inside, outside)
        self.humiditySource = humiditySource
        ## Humidity observation, percent
        self.humidity = humidity
        DataPacket.__init__(self, date, timestamp, elapsed_time)

    ## Provide the fixed-text string name for this data packet
    #
    # This simply reports the human-readable name for the class so that reporting is possible
    #
    # \param self   Pointer to the object
    # \return String with the human-readable name of the packet
    def name(self):
        return "Humidity"

    ## Implement the printable interface for this class, allowing it to be streamed
    #
    # This converts to human-readable version of the data packet for the standard streaming output interface.
    #
    # \param self   Pointer to the object
    # \return String representation of the object
    def __str__(self):
        rtn = DataPacket.__str__(self) + " " + self.name() + ": humidity = " + str(self.humidity) + " % (source "\
              + str(self.humiditySource) + ")"
        return rtn

## Implement the Pressure NMEA2000 message
#
# The Pressure message can serve a number of purposes, carrying pressure information for a variety of different sensors
# on the ship, including atmospheric and compressed air systems.  The information is, however, always qualified with a
# source designator.  Some filtering of messages might happen at the logger, however, which means that not all pressure
# messages make it to here.
class Pressure(DataPacket):
    ## Initialise the Pressure message
    #
    # This picks out the date and time of message reception (based on the last known good real time estimate), and the
    # pressure source and pressure as (u16, double, u8, double) for 19B total.  Pressure source is a mapped enum (see
    # Wiki for details); pressure is in Pascals.
    #
    # \param self   Pointer to the object
    # \param buffer Binary data from file to interpret
    def __init__(self, buffer):
        (date, timestamp, elapsed_time, pressureSource, pressure) = struct.unpack("<HdIBd", buffer)
        ## Source of pressure measurement (e.g., atmospheric, compressed air)
        self.pressureSource = pressureSource
        ## Pressure, Pascals
        self.pressure = pressure
        DataPacket.__init__(self, date, timestamp, elapsed_time)

    ## Provide the fixed-text string name for this data packet
    #
    # This simply reports the human-readable name for the class so that reporting is possible
    #
    # \param self   Pointer to the object
    # \return String with the human-readable name of the packet
    def name(self):
        return "Pressure"

    ## Implement the printable interface for this class, allowing it to be streamed
    #
    # This converts to human-readable version of the data packet for the standard streaming output interface.
    #
    # \param self   Pointer to the object
    # \return String representation of the object
    def __str__(self):
        rtn = DataPacket.__str__(self) + " " + self.name() + ": pressure = " + str(pressure_to_mbar(self.pressure))\
              + " mBar (source " + str(self.pressureSource) + ")"
        return rtn

## Implement the NMEA0183 serial data message
#
# As an extension, the logger can (if the hardware is populated) record data from two separate RS-422 NMEA0183
# data streams, and timestamp in the same manner as the rest of the data.  The code encapsulates the entire message
# in this packet, rather than trying to have a separate packet for each data string type (at least for now).
class SerialString(DataPacket):
    ## Initialise the SerialString message
    #
    # This picks out the date and time of message reception (based on the last known good real time estimate), and the
    # serial string.
    #
    # \param self   Pointer to the object
    # \param buffer Binary data from file to interpret
    def __init__(self, buffer):
        string_length = len(buffer) - 4
        convert_string = "<I" + str(string_length) + "s"
        (elapsed_time, payload) = struct.unpack(convert_string, buffer)
        ## Serial data encapsulated in the packet
        self.payload = payload
        DataPacket.__init__(self, 0, 0, elapsed_time)

    ## Provide the fixed-text string name for this data packet
    #
    # This simply reports the human-readable name for the class so that reporting is possible
    #
    # \param self   Pointer to the object
    # \return String with the human-readable name of the packet
    def name(self):
        return "SerialString"

    ## Implement the printable interface for this class, allowing it to be streamed
    #
    # This converts to human-readable version of the data packet for the standard streaming output interface.
    #
    # \param self   Pointer to the object
    # \return String representation of the object
    def __str__(self):
        rtn = DataPacket.__str__(self) + " " + self.name() + ": payload = " + str(self.payload)
        return rtn


##  Unpack the serialiser version information packet, and store versions
#
# This picks apart the information on the version of the serialiser used to generate the file being read.  This should
# always be the first packet in the file, and allows the code to adjust readers if necessary in order to read what's
# coming next.
class SerialiserVersion(DataPacket):
    ## Initialise the object using the supplied buffer of binary data
    #
    # The buffer should contain eight bytes as two unsigned integers for major and minor software versions
    #
    # \param self   Pointer to the object
    # \param buffer Binary data from file to interpret
    def __init__(self, buffer):
        (major, minor, n2000_major, n2000_minor, n2000_patch, n0183_major, n0183_minor, n0183_patch) = \
            struct.unpack("<HHHHHHHH", buffer)
        ## Major software version for the serialiser code
        self.major = major
        ## Minor software version for the serialiser code
        self.minor = minor
        ## NMEA2000 software version information
        self.nmea2000_version = str(n2000_major) + "." + str(n2000_minor) + "." + str(n2000_patch)
        ## NMEA0183 software version information
        self.nmea0183_version = str(n0183_major) + "." + str(n0183_minor) + "." + str(n0183_patch)
        DataPacket.__init__(self, 0, 0.0, 0)

    ## Provide the fixed-text string name for this data packet
    #
    # This simply reports the human-readable name for the class so that reporting is possible
    #
    # \param self   Pointer to the object
    # \return String with the human-readable name of the packet
    def name(self):
        return "SerialiserVersion"

    ## Implement the printable interface for this class, allowing it to be streamed
    #
    # This converts to human-readable version of the data packet for the standard streaming output interface.
    #
    # \param self   Pointer to the object
    # \return String representation of the object
    def __str__(self):
        rtn = DataPacket.__str__(self) + " " + self.name() + ": version = " + str(self.major) + "." + str(self.minor) +\
                " with NMEA2000 version " + self.nmea2000_version + " and NMEA0183 version " + self.nmea0183_version
        return rtn

## Implement the motion sensor data packet
#
# This picks out the information from the on-board motion sensor (if available).  This data is not processed
# (e.g., with a Kalman filter) and may need further work before being useful.
class Motion(DataPacket):
    ## Initialise the object using the supplied buffer of binary data
    #
    # The buffer should contain 28 bytes for 3-axis acceleration, 3-axis gyro, and internal sensor temperature.
    #
    # \param self   Pointer to the object
    # \param buffer Binary data from file to interpret
    def __init__(self, buffer):
        (elapsed, ax, ay, az, gx, gy, gz, temp) = struct.unpack("<Ifffffff", buffer)
        # Accelerations, 3-axis
        self.ax = ax
        self.ay = ay
        self.az = az
        # Gyroscope rotation rates, 3-axis
        self.gx = gx
        self.gy = gy
        self.gz = gz
        # Die temperature of the motion sensor
        self.temp = temp
        DataPacket.__init__(self, 0, 0.0, elapsed)

    ## Provide the fixed-text string name for this data pakcet
    #
    # This simply reports the human-readable name for the class so that reporting is possible
    #
    # \param self   Pointer to the object
    # \return String with the human-readable name of the packet
    def name(self):
        return "Motion"
    
    ## Implement the printable interface for this class, allowing it to be streamed
    #
    # This converts to human-readable version of the data packet for the standard streaming output interface.
    #
    # \param self   Pointer to the object
    # \return String representation of the object
    def __str__(self):
        rtn = DataPacket.__str__(self) + " " + self.name() + ": acc = (" + str(self.ax) + ", " + str(self.ay) + ", " + str(self.az) +\
            "), gyro = (" + str(self.gx) + ", " + str(self.gy) + ", " + str(self.gz) + "), temp = " + str(self.temp)
        return rtn

## Implement the metadata packet
#
# This picks out the information from the metadata packet, which gives identification
# information for the logger that created the file.
class Metadata(DataPacket):
    ## Initialise the object using the supplied buffer of binary data
    #
    # The buffer should contain two strings (of variable length)
    #
    # \param self   Pointer to the object
    # \param buffer Binary data from file to interpret
    def __init__(self, buffer):
        base = 0
        name_len, = struct.unpack_from("<I", buffer, base);
        base += 4
        convert_string = "<" + str(name_len) + "s"
        name, = struct.unpack_from(convert_string, buffer, base)
        base += name_len
        id_len, = struct.unpack_from("<I", buffer, base);
        base += 4
        convert_string = "<" + str(id_len) + "s"
        unique_id, = struct.unpack_from(convert_string, buffer, base);
        self.ship_name = name.decode("UTF-8")
        self.ship_id = unique_id.decode("UTF-8")
        DataPacket.__init__(self, 0, 0.0, 0)
    
    ## Provide the fixed-text string name for this data packet
    #
    # This simply reports the human-readable name for the class so that reporting is possible
    #
    # \param self   Pointer to the object
    # \return String with the human-readable name of the packet
    def name(self):
        return "Metadata"
    
    ## Implement the printable interface for this class, allowing it to be streamed
    #
    # This converts to human-readable version of the data packet for the standard streaming output interface
    #
    # \param self   Pointer to the object
    # \return String representation of the object
    def __str__(self):
        rtn = DataPacket.__str__(self) + " " + self.name() + ": ship name = " + self.ship_name + ", identifier = " + self.ship_id
        return rtn
        
## Translate packets out of the binary file, reconstituing as an appropriate class
#
# This provides the primary interface for the user to the binary data generated by the logger.  Calling the next_packet
# method pulls the next packet header, checks for type and size, and then reads the following byte sequence to the
# required length before translating to an instantiation of the appropriate class.  Unknown packets generate a warning.
class PacketFactory:
    ## Initialise the packet factory
    #
    # This simple copies the file reference information for the binary data, and resets EOF indicator.
    #
    # \param self   Pointer to the object
    # \param file   Open file object, which must be opened for binary reads
    def __init__(self, file):
        ## File reference from which to read packets
        self.file = file
        ## Flag for end-of-file detection
        self.end_of_file = False

    ## Extract the next packet from the binary data file
    #
    # This pulls the next packet header from the binary file, interprets the type and size, reads the bytes
    # corresponding to the packet payload, and the converts to an instantiation of the appropriate class object.
    #
    # \param self   Pointer to the object
    # \return DataPacket-derived object corresponding to the packet, or None if end-of-file or error
    def next_packet(self):
        if self.end_of_file:
            return None

        buffer = self.file.read(8)   # Header for each packet is U32 (ID) U32 (length in bytes)

        if len(buffer) < 8:
            #print("Failed to read 8-byte packet header in PacketFactory")
            self.end_of_file = True
            return None

        (pkt_id, pkt_len) = struct.unpack("<II", buffer)
        buffer = self.file.read(pkt_len)
        if pkt_id == 0:
            rtn = SerialiserVersion(buffer)
        elif pkt_id == 1:
            rtn = SystemTime(buffer)
        elif pkt_id == 2:
            rtn = Attitude(buffer)
        elif pkt_id == 3:
            rtn = Depth(buffer)
        elif pkt_id == 4:
            rtn = COG(buffer)
        elif pkt_id == 5:
            rtn = GNSS(buffer)
        elif pkt_id == 6:
            rtn = Environment(buffer)
        elif pkt_id == 7:
            rtn = Temperature(buffer)
        elif pkt_id == 8:
            rtn = Humidity(buffer)
        elif pkt_id == 9:
            rtn = Pressure(buffer)
        elif pkt_id == 10:
            rtn = SerialString(buffer)
        elif pkt_id == 11:
            rtn = Motion(buffer)
        elif pkt_id == 12:
            rtn = Metadata(buffer)
        else:
            print("Unknown packet with ID " + str(pkt_id) + " in input stream; ignored.")
            rtn = None

        return rtn

    ## Check for more data being available
    #
    # This checks for whether there is more data available in the file.
    #
    # \param self   Pointer to the object
    # \return True if there is more data to read, otherwise False
    def has_more(self):
        return not self.end_of_file
