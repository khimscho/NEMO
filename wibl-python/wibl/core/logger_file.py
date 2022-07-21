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
from abc import ABC, abstractmethod
import io
from enum import Enum


## Exception used to report bad keyword parameters when setting up a packet from scratch in code
class SpecificationError(Exception):
    pass

## Exception used to report a bad translation of a packet (rather than passing up a raw struct exception)
class PacketTranscriptionError(Exception):
    pass

## Enumeration of the identification numbers associated with the various packets in a WIBL file
class PacketTypes(Enum):
    ## Version information for the logger's file construction code, and the NMEA2000 and NMEA0183 loggers
    SerialiserVersion = 0
    ## NMEA2000 SystemTime information
    SystemTime = 1
    ## NMEA2000 Attitude (roll, pitch, yaw) information
    Attitude = 2
    ## NMEA2000 Depth information
    Depth = 3
    ## NMEA2000 Course-over-ground information
    COG = 4
    ## NMEA2000 GNSS report information
    GNSS = 5
    ## NMEA2000 Environmental (temperature, pressure, and humidity) information
    Environment = 6
    ## NMEA2000 Temperature information
    Temperature = 7
    ## NMEA2000 Humidity information
    Humidity = 8
    ## NMEA2000 Pressure information
    Pressure = 9
    ## Encapsulated NMEA0183 serial sentence
    SerialString = 10
    ## Local motion sensor (three-axis acceleration, three-axis gyro) information
    Motion = 11
    ## Logger and ship identification information used for construction GeoJSON metadata on output
    Metadata = 12
    ## Requests for algorithms to be run on the data in post-processing
    AlgorithmRequest = 13
    ## Arbitrary JSON metadata string used to fill in platform-specific items in the GeoJSON metadata on output
    JSONMetadata = 14
    ## Specification for a NMEA0183 packet to be recorded at the logger
    NMEA0183Filter = 15


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
class DataPacket(ABC):
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

    ## Abstract method for constructing the payload of the packet for serialisation
    #
    # This builds a buffer of the data required for the data packet so that the code can then serialise
    # it in new files.
    @abstractmethod
    def payload(self) -> bytes:
        pass

    ## Abstact method for a class to report its ID number
    #
    # Each packet written into the file has to have an ID number; the sub-class should know what this is.
    #
    @abstractmethod
    def id(self) -> int:
        pass

    ## Serialise the data in the current packet into the given file
    #
    # This wraps up the requirements to write a packet into a streamable binary output file.
    #
    # \param f  Binary output file
    def serialise(self, f: io.BufferedWriter) -> None:
        buffer = self.payload()
        id = self.id()
        #print(f'Writing packet with ID {id} and buffer length {len(buffer)}.')
        f.write(id.to_bytes(4, 'little'))
        f.write(len(buffer).to_bytes(4, 'little'))
        f.write(buffer)

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
    # \param kwargs Named keywords to initialise parameters, of "buffer" to unpack from binary data
    def __init__(self, **kwargs):
        if 'buffer' in kwargs:
            self.buffer_constructor(kwargs['buffer'])
        else:
            self.data_constructor(**kwargs)

    def buffer_constructor(self, buffer: bytes) -> None:
        (date, timestamp, elapsed_time, data_source) = struct.unpack('<HdIB', buffer)
        ## Source of the timestamp (see documentation for decoding, but at least GNSS)
        self.data_source = data_source
        DataPacket.__init__(self, date, timestamp, elapsed_time)

    def data_constructor(self, **kwargs) -> None:
        try:
            self.data_source = kwargs['data_source']
            super().__init__(kwargs['date'], kwargs['timestamp'], kwargs['elapsed_time'])
        except KeyError as e:
            raise SpecificationError('Bad packet parameters') from e
    
    def payload(self) -> bytes:
        buffer = struct.pack('<HdIB', self.date, self.timestamp, self.elapsed, self.data_source)
        return buffer
    
    def id(self) -> int:
        return PacketTypes.SystemTime.value

    ## Provide the fixed-text string name for this data packet
    #
    # This simply reports the human-readable name for the class so that reporting is possible
    #
    # \param self   Pointer to the object
    # \return String with the human-readable name of the packet
    def name(self):
        return 'SystemTime'

    ## Implement the printable interface for this class, allowing it to be streamed
    #
    # This converts to human-readable version of the data packet for the standard streaming output interface.
    #
    # \param self   Pointer to the object
    # \return String representation of the object
    def __str__(self):
        rtn = super().__str__() + ' ' + self.name() + ':  source = ' + str(self.data_source)
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
    # \param kwargs Named keywords to initialise parameters, of "buffer" to unpack from binary data
    def __init__(self, **kwargs):
        if 'buffer' in kwargs:
            self.buffer_constructor(kwargs['buffer'])
        else:
            self.data_constructor(**kwargs)

    def buffer_constructor(self, buffer: bytes) -> None:
        (date, timestamp, elapsed_time, yaw, pitch, roll) = struct.unpack("<HdIddd", buffer)
        ## Yaw angle of the ship, radians (+ve clockwise from north)
        self.yaw = yaw
        ## Pitch angle of the ship, radians (+ve bow up)
        self.pitch = pitch
        ## Roll angle of the ship, radians (+ve port up)
        self.roll = roll
        DataPacket.__init__(self, date, timestamp, elapsed_time)

    def payload(self) -> bytes:
        buffer = struct.pack('<HdIddd', self.date, self.timestamp, self.elapsed, self.yaw, self.pitch, self.roll)
        return buffer
    
    def data_constructor(self, **kwargs) -> None:
        try:
            self.yaw = kwargs['yaw']
            self.pitch = kwargs['pitch']
            self.roll = kwargs['roll']
            super().__init__(kwargs['date'], kwargs['timestamp'], kwargs['elapsed_time'])
        except KeyError as e:
            raise SpecificationError('Bad packet parameters') from e

    def id(self) -> int:
        return PacketTypes.Attitude.value

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
        rtn = super().__str__() + " " + self.name() + ": yaw = " + str(angle_to_degs(self.yaw))\
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
    # \param kwargs Named keywords to initialise parameters, of "buffer" to unpack from binary data
    def __init__(self, **kwargs):
        if 'buffer' in kwargs:
            self.buffer_constructor(kwargs['buffer'])
        else:
            self.data_constructor(**kwargs)

    def buffer_constructor(self, buffer: bytes) -> None:
        (date, timestamp, elapsed_time, depth, offset, range) = struct.unpack('<HdIddd', buffer)
        ## Observed depth below transducer, metres
        self.depth = depth
        ## Offset for depth, metres.
        # This is an offset to apply to reference the depth to either the water surface, or the keel.  Positive
        # values imply that the correction is for water surface to transducer; negative implies transducer to keel
        self.offset = offset
        ## Maximum range of observation, metres
        self.range = range
        super().__init__(date, timestamp, elapsed_time)

    def payload(self) -> bytes:
        buffer = struct.pack('<HdIddd', self.date, self.timestamp, self.elapsed, self.depth, self.offset, self.range)
        return buffer
    
    def id(self) -> int:
        return PacketTypes.Depth.value
    
    def data_constructor(self, **kwargs) -> None:
        try:
            self.depth = kwargs['depth']
            self.offset = kwargs['offset']
            self.range = kwargs['range']
            super().__init__(kwargs['date'], kwargs['timestamp'], kwargs['elapsed_time'])
        except KeyError as e:
            raise SpecificationError('Bad packet parameters') from e

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
        rtn = super().__str__() + ' ' + self.name() + ': depth = ' + str(self.depth) + 'm, offset = '\
              + str(self.offset) + 'm, range = ' + str(self.range) + 'm'
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
    # \param kwargs Named keywords to initialise parameters, of "buffer" to unpack from binary data
    def __init__(self, **kwargs):
        if 'buffer' in kwargs:
            self.buffer_constructor(kwargs['buffer'])
        else:
            self.data_constructor(**kwargs)

    def buffer_constructor(self, buffer: bytes) -> None:
        (date, timestamp, elapsed_time, courseOverGround, speedOverGround) = struct.unpack('<HdIdd', buffer)
        ## Course over ground (radians)
        self.courseOverGround = courseOverGround
        ## Speed over ground (m/s)
        self.speedOverGround = speedOverGround
        super().__init__(date, timestamp, elapsed_time)
    
    def payload(self) -> bytes:
        buffer = struct.pack('<HdIdd', self.date, self.timestamp, self.elapsed, self.courseOverGround, self.speedOverGround)
        return buffer

    def id(self) -> int:
        return PacketTypes.COG.value
    
    def data_constructor(self, **kwargs) -> None:
        try:
            self.courseOverGround = kwargs['cog']
            self.speedOverGround = kwargs['sog']
            super().__init__(kwargs['date'], kwargs['timestamp'], kwargs['elapsed_time'])
        except KeyError as e:
            raise SpecificationError('Bad packet parameters') from e

    ## Provide the fixed-text string name for this data packet
    #
    # This simply reports the human-readable name for the class so that reporting is possible
    #
    # \param self   Pointer to the object
    # \return String with the human-readable name of the packet
    def name(self):
        return 'Course Over Ground'

    ## Implement the printable interface for this class, allowing it to be streamed
    #
    # This converts to human-readable version of the data packet for the standard streaming output interface.
    #
    # \param self   Pointer to the object
    # \return String representation of the object
    def __str__(self):
        rtn = super().__str__() + ' ' + self.name() + ': course over ground = '\
              + str(angle_to_degs(self.courseOverGround)) + ' deg, speed over ground = '\
              + str(self.speedOverGround) + ' m/s'
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
    # \param kwargs Named keywords to initialise parameters, of "buffer" to unpack from binary data
    def __init__(self, **kwargs):
        if 'buffer' in kwargs:
            self.buffer_constructor(kwargs['buffer'])
        else:
            self.data_constructor(**kwargs)
        
    def buffer_constructor(self, buffer: bytes) -> None:
        (sys_date, sys_timestamp, sys_elapsed, date, timestamp, latitude, longitude, altitude,
         receiverType, receiverMethod, numSVs, horizontalDOP, positionDOP, separation, numRefStations, refStationType,
         refStationID, correctionAge) = struct.unpack('<HdIHddddBBBdddBBHd', buffer)
        ## In-message date (days since epoch)
        self.msg_date = date
        ## In-message timestamp (seconds since midnight)
        self.msg_timestamp = timestamp
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
        super().__init__(sys_date, sys_timestamp, sys_elapsed)
    
    def payload(self) -> bytes:
        buffer = struct.pack('<HdIHddddBBBdddBBHd', self.date, self.timestamp, self.elapsed, self.msg_date, self.msg_timestamp,
                                                    self.latitude, self.longitude, self.altitude, self.receiverType, self.receiverMethod,
                                                    self.numSVs, self.horizontalDOP, self.positionDOP, self.separation,
                                                    self.numRefStations, self.refStationType, self.refStationID, self.correctionAge)
        return buffer
    
    def id(self) -> int:
        return PacketTypes.GNSS.value

    def data_constructor(self, **kwargs) -> None:
        try:
            self.msg_date = kwargs['msg_date']
            self.msg_timestamp = kwargs['msg_timestamp']
            self.latitude = kwargs['latitude']
            self.longitude = kwargs['longitude']
            self.altitude = kwargs['altitude']
            self.receiverType = kwargs['rx_type']
            self.receiverMethod = kwargs['rx_method']
            self.numSVs = kwargs['num_svs']
            self.horizontalDOP = kwargs['horizontal_dop']
            self.positionDOP = kwargs['position_dop']
            self.separation = kwargs['sep']
            self.numRefStations = kwargs['n_refs']
            self.refStationType = kwargs['refs_type']
            self.refStationID = kwargs['refs_id']
            self.correctionAge = kwargs['correction_age']
            super().__init__(kwargs['date'], kwargs['timestamp'], kwargs['elapsed_time'])
        except KeyError as e:
            raise SpecificationError('Bad packet parameters') from e

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
        rtn = super().__str__() + " " + self.name() + ": in-message date = " + str(self.msg_date) + " days, "\
              + "in-message time = " + str(self.msg_timestamp) + " s.,  latitude = " + str(self.latitude)\
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
    # \param kwargs Named keywords to initialise parameters, of "buffer" to unpack from binary data
    def __init__(self, **kwargs):
        if 'buffer' in kwargs:
            self.buffer_constructor(kwargs['buffer'])
        else:
            self.data_constructor(**kwargs)
        
    def buffer_constructor(self, buffer: bytes) -> None:
        (date, timestamp, elapsed_time, tempSource, temperature, humiditySource, humidity, pressure) = \
            struct.unpack('<HdIBdBdd', buffer)
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
        super().__init__(date, timestamp, elapsed_time)

    def payload(self) -> bytes:
        buffer = struct.pack('<HdIBdBdd', self.date, self.timestamp, self.elapsed, self.tempSource, self.temperature, self.humiditySource, self.humidity, self.pressure)
        return buffer

    def id(self) -> int:
        return PacketTypes.Environment.value

    def data_constructor(self, **kwargs) -> None:
        try:
            self.tempSource = kwargs['temp_source']
            self.temperature = kwargs['temp']
            self.humiditySource = kwargs['humid_source']
            self.humidity = kwargs['humidity']
            self.pressure = kwargs['pressure']
            super().__init__(kwargs['date'], kwargs['timestamp'], kwargs['elapsed_time'])
        except KeyError as e:
            raise SpecificationError('Bad packet parameters') from e

    ## Provide the fixed-text string name for this data packet
    #
    # This simply reports the human-readable name for the class so that reporting is possible
    #
    # \param self   Pointer to the object
    # \return String with the human-readable name of the packet
    def name(self):
        return 'Environment'

    ## Implement the printable interface for this class, allowing it to be streamed
    #
    # This converts to human-readable version of the data packet for the standard streaming output interface.
    #
    # \param self   Pointer to the object
    # \return String representation of the object
    def __str__(self):
        rtn = super().__str__() + ' ' + self.name() + ': temperature = ' + str(temp_to_celsius(self.temperature)) + ' ºC (source '\
              + str(self.tempSource) + '), humidity = ' + str(self.humidity) + '% (source ' + str(self.humiditySource) +\
              '), pressure = ' + str(pressure_to_mbar(self.pressure)) + ' mBar'
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
    # \param kwargs Named keywords to initialise parameters, of "buffer" to unpack from binary data
    def __init__(self, **kwargs):
        if 'buffer' in kwargs:
            self.buffer_constructor(kwargs['buffer'])
        else:
            self.data_constructor(**kwargs)
    
    def buffer_constructor(self, buffer: bytes) -> None:
        (date, timestamp, elapsed_time, tempSource, temperature) = struct.unpack('<HdIBd', buffer)
        ## Source of temperature information (e.g., water, air, cabin)
        self.tempSource = tempSource
        ## Temperature of source, Kelvin
        self.temperature = temperature
        super().__init__(date, timestamp, elapsed_time)

    def payload(self) -> bytes:
        buffer = struct.pack('<HdIBd', self.date, self.timestamp, self.elapsed, self.tempSource, self.tempSource)
        return buffer

    def id(self) -> int:
        return PacketTypes.Temperature.value

    def data_constructor(self, **kwargs) -> None:
        try:
            self.tempSource = kwargs['temp_source']
            self.temperature = kwargs['temp']
            super().__init__(kwargs['date'], kwargs['timestamp'], kwargs['elapsed_time'])
        except KeyError as e:
            raise SpecificationError('Bad packet parameters') from e

    ## Provide the fixed-text string name for this data packet
    #
    # This simply reports the human-readable name for the class so that reporting is possible
    #
    # \param self   Pointer to the object
    # \return String with the human-readable name of the packet
    def name(self):
        return 'Temperature'

    ## Implement the printable interface for this class, allowing it to be streamed
    #
    # This converts to human-readable version of the data packet for the standard streaming output interface.
    #
    # \param self   Pointer to the object
    # \return String representation of the object
    def __str__(self):
        rtn = super().__str__() + ' ' + self.name() + ': temperature = ' + str(temp_to_celsius(self.temperature))\
              + ' ºC (source ' + str(self.tempSource) + ')'
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
    # \param kwargs Named keywords to initialise parameters, of "buffer" to unpack from binary data
    def __init__(self, **kwargs):
        if 'buffer' in kwargs:
            self.buffer_constructor(kwargs['buffer'])
        else:
            self.data_constructor(**kwargs)
    
    def buffer_constructor(self, buffer: bytes) -> None:
        (date, timestamp, elapsed_time, humiditySource, humidity) = struct.unpack('<HdIBd', buffer)
        ## Source of humidity (e.g., inside, outside)
        self.humiditySource = humiditySource
        ## Humidity observation, percent
        self.humidity = humidity
        super().__init__(date, timestamp, elapsed_time)
    
    def payload(self) -> bytes:
        buffer = struct.pack('<HdIBd', self.date, self.timestamp, self.elapsed, self.humiditySource, self.humidity)
        return buffer
    
    def id(self) -> int:
        return PacketTypes.Humidity.value

    def data_constructor(self, **kwargs):
        try:
            self.humiditySource = kwargs['humid_source']
            self.humidity = kwargs['humidity']
            super().__init__(kwargs['date'], kwargs['timestamp'], kwargs['elapsed_time'])
        except KeyError as e:
            raise SpecificationError('Bad packet parameters') from e

    ## Provide the fixed-text string name for this data packet
    #
    # This simply reports the human-readable name for the class so that reporting is possible
    #
    # \param self   Pointer to the object
    # \return String with the human-readable name of the packet
    def name(self):
        return 'Humidity'

    ## Implement the printable interface for this class, allowing it to be streamed
    #
    # This converts to human-readable version of the data packet for the standard streaming output interface.
    #
    # \param self   Pointer to the object
    # \return String representation of the object
    def __str__(self):
        rtn = super().__str__() + ' ' + self.name() + ': humidity = ' + str(self.humidity) + ' % (source '\
              + str(self.humiditySource) + ')'
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
    # \param kwargs Named keywords to initialise parameters, of "buffer" to unpack from binary data
    def __init__(self, **kwargs):
        if 'buffer' in kwargs:
            self.buffer_constructor(kwargs['buffer'])
        else:
            self.data_constructor(**kwargs)

    def buffer_constructor(self, buffer: bytes) -> None:
        (date, timestamp, elapsed_time, pressureSource, pressure) = struct.unpack('<HdIBd', buffer)
        ## Source of pressure measurement (e.g., atmospheric, compressed air)
        self.pressureSource = pressureSource
        ## Pressure, Pascals
        self.pressure = pressure
        super().__init__(date, timestamp, elapsed_time)
    
    def payload(self) -> bytes:
        buffer = struct.pack('<HdIBd', self.date, self.timestamp, self.elapsed, self.pressureSource, self.pressure)
        return buffer
    
    def id(self) -> int:
        return PacketTypes.Pressure.value

    def data_constructor(self, **kwargs):
        try:
            self.pressureSource = kwargs['press_source']
            self.pressure = kwargs['pressure']
            super().__init__(kwargs['date'], kwargs['timestamp'], kwargs['elapsed_time'])
        except KeyError as e:
            raise SpecificationError('Bad packet parameters') from e
    
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
        rtn = super().__str__() + " " + self.name() + ": pressure = " + str(pressure_to_mbar(self.pressure))\
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
    # \param kwargs Named keywords to initialise parameters, of "buffer" to unpack from binary data
    def __init__(self, **kwargs):
        if 'buffer' in kwargs:
            self.buffer_constructor(kwargs['buffer'])
        else:
            self.data_constructor(**kwargs)

    def buffer_constructor(self, buffer: bytes) -> None:
        string_length = len(buffer) - 4
        convert_string = '<I' + str(string_length) + 's'
        (elapsed_time, data) = struct.unpack(convert_string, buffer)
        ## Serial data encapsulated in the packet
        self.data = data
        super().__init__(0, 0, elapsed_time)

    def payload(self) -> bytes:
        data_len = len(self.data)
        buffer = struct.pack(f'<I{data_len}s', self.elapsed, self.data)
        return buffer

    def id(self) -> int:
        return PacketTypes.SerialString.value

    def data_constructor(self, **kwargs) -> None:
        try:
            self.data = kwargs['payload']
            super().__init__(0, 0, kwargs['elapsed_time'])
        except KeyError as e:
            raise SpecificationError('Bad packet parameters') from e

    ## Provide the fixed-text string name for this data packet
    #
    # This simply reports the human-readable name for the class so that reporting is possible
    #
    # \param self   Pointer to the object
    # \return String with the human-readable name of the packet
    def name(self):
        return 'SerialString'

    ## Implement the printable interface for this class, allowing it to be streamed
    #
    # This converts to human-readable version of the data packet for the standard streaming output interface.
    #
    # \param self   Pointer to the object
    # \return String representation of the object
    def __str__(self):
        rtn = super().__str__() + ' ' + self.name() + ': payload = ' + str(self.data)
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
    # \param kwargs Named keywords to initialise parameters, of "buffer" to unpack from binary data
    def __init__(self, **kwargs):
        if 'buffer' in kwargs:
            self.buffer_constructor(kwargs['buffer'])
        else:
            self.data_constructor(**kwargs)

    def buffer_constructor(self, buffer: bytes) -> None:
        (major, minor, n2000_major, n2000_minor, n2000_patch, n0183_major, n0183_minor, n0183_patch) = \
            struct.unpack('<HHHHHHHH', buffer)
        ## Major software version for the serialiser code
        self.major = major
        ## Minor software version for the serialiser code
        self.minor = minor
        ## A tuple of the NMEA2000 software version
        self.nmea2000 = (n2000_major, n2000_minor, n2000_patch)
        ## A tuple of the NMEA0183 software version
        self.nmea0183 = (n0183_major, n0183_minor, n0183_patch)
        ## NMEA2000 software version information
        self.nmea2000_version = str(n2000_major) + '.' + str(n2000_minor) + '.' + str(n2000_patch)
        ## NMEA0183 software version information
        self.nmea0183_version = str(n0183_major) + '.' + str(n0183_minor) + '.' + str(n0183_patch)

        super().__init__(0, 0.0, 0)

    def payload(self) -> bytes:
        buffer = struct.pack('<HHHHHHHH', self.major, self.minor, self.nmea2000[0], self.nmea2000[1], self.nmea2000[2],
                            self.nmea0183[0], self.nmea0183[1], self.nmea0183[2])
        return buffer
    
    def id(self) -> int:
        return PacketTypes.SerialiserVersion.value
    
    def data_constructor(self, **kwargs) -> None:
        try:
            self.major = kwargs['major']
            self.minor = kwargs['minor']
            self.nmea2000 = kwargs['n2000']
            self.nmea0183 = kwargs['n0183']
            self.nmea2000_version = str(self.nmea2000[0]) + '.' + str(self.nmea2000[1]) + '.' + str(self.nmea2000[2])
            self.nmea0183_version = str(self.nmea0183[0]) + '.' + str(self.nmea0183[1]) + '.' + str(self.nmea0183[2])
            super().__init__(0, 0.0, 0)
        except KeyError as e:
            raise SpecificationError('Bad packet parameters') from e

    ## Provide the fixed-text string name for this data packet
    #
    # This simply reports the human-readable name for the class so that reporting is possible
    #
    # \param self   Pointer to the object
    # \return String with the human-readable name of the packet
    def name(self):
        return 'SerialiserVersion'

    ## Implement the printable interface for this class, allowing it to be streamed
    #
    # This converts to human-readable version of the data packet for the standard streaming output interface.
    #
    # \param self   Pointer to the object
    # \return String representation of the object
    def __str__(self):
        rtn = super().__str__() + ' ' + self.name() + f': version = {self.major}.{self.minor}, with NMEA2000 version {self.nmea2000} and NMEA0183 version {self.nmea0183}'
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
    # \param kwargs Named keywords to initialise parameters, of "buffer" to unpack from binary data
    def __init__(self, **kwargs):
        if 'buffer' in kwargs:
            self.buffer_constructor(kwargs['buffer'])
        else:
            self.data_constructor(**kwargs)

    def buffer_constructor(self, buffer: bytes) -> None:
        (elapsed, ax, ay, az, gx, gy, gz, temp) = struct.unpack('<Ifffffff', buffer)
        ## The acceleration vector, 3D
        self.accel = (ax, ay, az)
        ## The gyroscope rate vector, 3D
        self.gyro = (gx, gy, gz)
        # Die temperature of the motion sensor
        self.temp = temp
        super().__init__(0, 0.0, elapsed)

    def payload(self) -> bytes:
        buffer = struct.pack('<Ifffffff', self.elapsed, self.accel[0], self.accel[1], self.accel[2], self.gyro[0], self.gyro[1], self.gyro[2], self.temp)
        return buffer
    
    def id(self) -> int:
        return PacketTypes.Motion.value

    def data_constructor(self, **kwargs) -> None:
        try:
            self.accel = kwargs['accel']
            self.gyro = kwargs['gyro']
            super().__init__(0, 0.0, kwargs['elapsed_time'])
        except KeyError as e:
            raise SpecificationError('Bad packet parameters') from e

    ## Provide the fixed-text string name for this data pakcet
    #
    # This simply reports the human-readable name for the class so that reporting is possible
    #
    # \param self   Pointer to the object
    # \return String with the human-readable name of the packet
    def name(self):
        return 'Motion'
    
    ## Implement the printable interface for this class, allowing it to be streamed
    #
    # This converts to human-readable version of the data packet for the standard streaming output interface.
    #
    # \param self   Pointer to the object
    # \return String representation of the object
    def __str__(self):
        rtn = super().__str__() + ' ' + self.name() + ': acc = ' + str(self.accel) + ', gyro = ' + str(self.gyro) + ', temp = ' + str(self.temp)
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
    # \param kwargs Named keywords to initialise parameters, of "buffer" to unpack from binary data
    def __init__(self, **kwargs):
        if 'buffer' in kwargs:
            self.buffer_constructor(kwargs['buffer'])
        else:
            self.data_constructor(**kwargs)

    def buffer_constructor(self, buffer: bytes) -> None:
        base = 0
        name_len, = struct.unpack_from('<I', buffer, base)
        base += 4
        convert_string = '<' + str(name_len) + 's'
        name, = struct.unpack_from(convert_string, buffer, base)
        base += name_len
        id_len, = struct.unpack_from('<I', buffer, base)
        base += 4
        convert_string = '<' + str(id_len) + 's'
        unique_id, = struct.unpack_from(convert_string, buffer, base)
        self.logger_name = name.decode('UTF-8')
        self.ship_name = unique_id.decode('UTF-8')
        super().__init__(0, 0.0, 0)

    def payload(self) -> bytes:
        logger_name_len = len(self.logger_name)
        ship_name_len = len(self.ship_name)
        buffer = struct.pack(f'<I{logger_name_len}sI{ship_name_len}s', logger_name_len, self.logger_name.encode('UTF-8'), ship_name_len, self.ship_name.encode('UTF-8'))
        return buffer
    
    def id(self) -> int:
        return PacketTypes.Metadata.value

    def data_constructor(self, **kwargs) -> None:
        try:
            self.logger_name = kwargs['logger']
            self.ship_name = kwargs['uniqid']
            super().__init__(0, 0.0, 0)
        except KeyError as e:
            raise SpecificationError('Bad packet parameters') from e

    
    ## Provide the fixed-text string name for this data packet
    #
    # This simply reports the human-readable name for the class so that reporting is possible
    #
    # \param self   Pointer to the object
    # \return String with the human-readable name of the packet
    def name(self):
        return 'Metadata'
    
    ## Implement the printable interface for this class, allowing it to be streamed
    #
    # This converts to human-readable version of the data packet for the standard streaming output interface
    #
    # \param self   Pointer to the object
    # \return String representation of the object
    def __str__(self):
        rtn = DataPacket.__str__(self) + ' ' + self.name() + ': logger name = ' + self.logger_name + ', identifier = ' + self.ship_name
        return rtn

## Implement the algorithm packet
#
# This picks out the information from the algorithm request packet, which provides an algorithm name
# and parameter set that the logger would recommend running on the data in the cloud, if available
class AlgorithmRequest(DataPacket):
    ## Initialise the object using the supplied buffer of binary data
    #
    # The buffer should contain two strings (or variable length), the first the algorithm name,
    # and the second the associated parameters (in whatever form the algorithm requires).
    #
    # \param self   Pointer to the object
    # \param kwargs Named keywords to initialise parameters, of "buffer" to unpack from binary data
    def __init__(self, **kwargs):
        if 'buffer' in kwargs:
            self.buffer_constructor(kwargs['buffer'])
        else:
            self.data_constructor(**kwargs)

    def buffer_constructor(self, buffer: bytes) -> None:
        base = 0
        algname_len, = struct.unpack_from('<I', buffer, base)
        base += 4
        convert_string = '<' + str(algname_len) + 's'
        algname, = struct.unpack_from(convert_string, buffer, base)
        base += algname_len
        param_len, = struct.unpack_from('<I', buffer, base)
        base += 4
        convert_string = '<' + str(param_len) + 's'
        algparams, = struct.unpack_from(convert_string, buffer, base)
        self.algorithm = algname
        self.parameters = algparams
        super().__init__(0, 0.0, 0)
    
    def payload(self) -> bytes:
        name_len = len(self.algorithm)
        param_len = len(self.parameters)
        buffer = struct.pack(f'<I{name_len}sI{param_len}s', name_len, self.algorithm, param_len, self.parameters)
        return buffer
    
    def id(self) -> int:
        return PacketTypes.AlgorithmRequest.value

    def data_constructor(self, **kwargs) -> None:
        try:
            self.algorithm = kwargs['name'].encode('UTF-8')
            self.parameters = kwargs['params'].encode('UTF-8')
            super().__init__(0, 0.0, 0)
        except KeyError as e:
            raise SerialiserVersion('Bad packet parameters') from e
    
    ## Provide the fixed-text string name for this data packet
    #
    # This simply report the human-readable name for the class so that reporting is possible
    #
    # \param self   Pointer to the object
    # \return String with the human-readable name of the packet
    def name(self):
        return 'AlgorithmRequest'
        
    ## Implement the printable interface for this class, allowing it to be streamed
    #
    # This converts to human-readable version of the data pacet for the standard streaming output interface
    #
    # \param self   Pointer to the object
    # \return String representation of the obect
    def __str__(self):
        rtn = DataPacket.__str__(self) + ' ' + self.name() + ': algorithm = ' + str(self.algorithm) + ', parameters = ' + str(self.parameters)
        return rtn

## Implement the JSON metadata packet
#
# This picks out information on metadata elements that the logger would like to send into the JSON
# file being constructed for each data file being transmitted to the database.  This is provided by
# the user and cached on the logger, and then transmitted as is, without interpretation.
class JSONMetadata(DataPacket):
    ## Initialise the object using the supplied buffer of binary data
    #
    # The buffer should contain a single string, which may contain any printable content.  This is
    # intended, however, to the JSON for the "platform" component of the GeoJSON metadata to be
    # passed to the database.
    #
    # \param self   Pointer to the object
    # \param kwargs Named keywords to initialise parameters, of "buffer" to unpack from binary data
    def __init__(self, **kwargs):
        if 'buffer' in kwargs:
            self.buffer_constructor(kwargs['buffer'])
        else:
            self.data_constructor(**kwargs)

    def buffer_constructor(self, buffer: bytes) -> None:
        base = 0
        meta_len, = struct.unpack_from('<I', buffer, base)
        base += 4
        convert_string = '<' + str(meta_len) + 's'
        meta, = struct.unpack_from(convert_string, buffer, base)
        self.metadata_element = meta
        super().__init__(0, 0.0, 0)

    def payload(self) -> bytes:
        meta_len = len(self.metadata_element)
        buffer = struct.pack(f'<I{meta_len}s', meta_len, self.metadata_element)
        return buffer
    
    def id(self) -> int:
        return PacketTypes.JSONMetadata.value

    def data_constructor(self, **kwargs) -> None:
        try:
            if type(kwargs['meta']) != 'bytes':
                self.metadata_element = kwargs['meta'].encode('UTF-8')
            else:
                self.metadata_element = kwargs['meta']
            super().__init__(0, 0.0, 0)
        except KeyError as e:
            raise SpecificationError('Bad packet parameters') from e
        
    ## Provide the fixed-text string name for this data packet
    #
    # This simply reports the human-readable name for the class so that reporting is possible
    def name(self):
        return 'JSONMetadata'
    
    ## Implement the printable interface for this class, allowing it to be streamed
    #
    # This converts to human-readable version of the data packet for the standard streaming output interface
    #
    # \param self
    # \return String representation of the object
    def __str__(self):
        rtn = DataPacket.__str__(self) + ' ' + self.name() + ': metadata element = \"' + self.metadata_element.decode('UTF-8') + '\"'
        return rtn

## Implement a packet to hold information on NMEA0183 packets being recorded
#
# The logger has the ability to filter the NMEA0183 sentences that are received so that it only  records to
# SD card those that are of interest.  Getting the filtering right can be important to let the capture run
# for as long as possible. 
class NMEA0183Filter(DataPacket):
    ## Initialise the object using the supplied buffer of data, or keywords if appropriate
    #
    # If the keywords include "buffer", the code assumes that the contents of the buffer are a serialised
    # version of the packet, and attempts to unpack it.  Otherwise, the code assumes that the keywords contain
    # information required to initialise the packet, and attempts to pull them from the dictionary.
    #
    # \param self   Reference for the object
    # \param kwargs Named arguments to initialise parameters, or "buffer" to unpack from binary data
    def __init__(self, **kwargs):
        if 'buffer' in kwargs:
            self.buffer_constructor(kwargs['buffer'])
        else:
            self.data_constructor(**kwargs)
    
    ## Initialise the packet from a binary buffer
    #
    # This takes the binary buffer presented and unpacks into an instance of the packet.
    #
    # \param self   Reference for the object
    # \param buffer Binary buffer with serialised information for the packet
    def buffer_constructor(self, buffer: bytes) -> None:
        base = 0
        id_len, = struct.unpack_from('<I', buffer, base)
        base += 4
        recog_string, = struct.unpack_from(f'<{id_len}s', buffer, base)
        self.recog_string = recog_string
        super().__init__(0, 0.0, 0)
    
    ## Initialise the packet from keyword arguments
    #
    # This takes the keywords provided and attempts to initialise the packet.  For this packet, valid
    # keywords are:
    #   'sentence': String containing the three-letter sentence name to be accepted for recording
    #
    # \param self       Reference for the object
    # \param **kwargs   Keyword dictionary with parameters for the packet
    def data_constructor(self, **kwargs) -> None:
        try:
            self.recog_string = kwargs['sentence'].encode('UTF-8')
            super().__init__(0, 0.0, 0)
        except KeyError as e:
            raise SpecificationError('Bad packet parameters') from e
    
    ## Encode the current packet for serialisation
    #
    # From the parameters set in the packet, convert to a stream of bytes that can be used to serialise
    # the packet to an output stream.  Note that this returns only the contents of the packet; the
    # packet length, recognition ID, etc., must be added separately.
    #
    # \param self   Reference for the object
    # \return Bytes array with the binary representation of the packet-specific parameters
    def payload(self) -> bytes:
        recog_len = len(self.recog_string)
        buffer = struct.pack(f'<I{recog_len}s', recog_len, self.recog_string)
        return buffer
    
    ## Provide the recognition ID for the packet, as used in the binary file
    #
    # Each packet has a reference number that's used as an ID; this routine provides that number
    #
    # \param self   Reference for the object
    # \returns Integer identification number for the packet
    def id(self) ->int:
        return PacketTypes.NMEA0183Filter.value

    ## Provide the fixed-text string name for this data packet
    #
    # This simply reports the human-readable name for the class so that reporting is possible
    #
    # \param self   Reference for the object
    # \return String with th ename of the object
    def name(self) -> str:
        return 'NMEA0183Filter'

    ## Implement the printable interface for this class, allowing it to be streamed
    #
    # This converts to human-readable version of the data packet for standard streaming output interface
    #
    # \param self   Reference for the object
    # \return String representation of the object
    def __str__(self) -> str:
        rtn = DataPacket.__str__(self) + ' ' + self.name() + ': sentence recognition string = \"' + self.recog_string.decode('UTF-8') + '\"'
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
        #print(f'Packet ID {pkt_id}, packet length {pkt_len}.')
        buffer = self.file.read(pkt_len)
        try:
            if pkt_id == PacketTypes.SerialiserVersion.value:
                rtn = SerialiserVersion(buffer=buffer)
            elif pkt_id == PacketTypes.SystemTime.value:
                rtn = SystemTime(buffer=buffer)
            elif pkt_id == PacketTypes.Attitude.value:
                rtn = Attitude(buffer=buffer)
            elif pkt_id == PacketTypes.Depth.value:
                rtn = Depth(buffer=buffer)
            elif pkt_id == PacketTypes.COG.value:
                rtn = COG(buffer=buffer)
            elif pkt_id == PacketTypes.GNSS.value:
                rtn = GNSS(buffer=buffer)
            elif pkt_id == PacketTypes.Environment.value:
                rtn = Environment(buffer=buffer)
            elif pkt_id == PacketTypes.Temperature.value:
                rtn = Temperature(buffer=buffer)
            elif pkt_id == PacketTypes.Humidity.value:
                rtn = Humidity(buffer=buffer)
            elif pkt_id == PacketTypes.Pressure.value:
                rtn = Pressure(buffer=buffer)
            elif pkt_id == PacketTypes.SerialString.value:
                rtn = SerialString(buffer=buffer)
            elif pkt_id == PacketTypes.Motion.value:
                rtn = Motion(buffer=buffer)
            elif pkt_id == PacketTypes.Metadata.value:
                rtn = Metadata(buffer=buffer)
            elif pkt_id == PacketTypes.AlgorithmRequest.value:
                rtn = AlgorithmRequest(buffer=buffer)
            elif pkt_id == PacketTypes.JSONMetadata.value:
                rtn = JSONMetadata(buffer=buffer)
            elif pkt_id == PacketTypes.NMEA0183Filter.value:
                rtn = NMEA0183Filter(buffer=buffer)
            else:
                print("Unknown packet with ID " + str(pkt_id) + " in input stream; ignored.")
                rtn = None
        except struct.error:
            raise PacketTranscriptionError

        return rtn

    ## Check for more data being available
    #
    # This checks for whether there is more data available in the file.
    #
    # \param self   Pointer to the object
    # \return True if there is more data to read, otherwise False
    def has_more(self):
        return not self.end_of_file
