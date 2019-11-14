# \file LoggerFile.py
# \brief Library objects for reading Seabed 2030 data logger files
#
# The Seabed 2030 low-cost logger generates files from NMEA2000 onto the SD card in
# fairly efficient binary format, with a timestamp from the local machine.  The code
# here unpacks this format and makes the data available.
#
# Copyright 2019, University of New Hampshire, Center for Coastal and Ocean Mapping.
# All Rights Reserved.

import struct


def temp_to_celsius(temp):
    return temp - 273.15


def pressure_to_mbar(pressure):
    return pressure / 100.0


def angle_to_degs(rads):
    return rads*180.0/3.1415926535897932384626433832795


class DataPacket:
    def __init__(self, date, timestamp):
        self.date = date
        self.timestamp = timestamp

    def __str__(self):
        rtn = "[" + str(self.date) + " days, " + str(self.timestamp) + " s.]"
        return rtn

class SystemTime(DataPacket):
    def __init__(self, buffer):
        (date, timestamp, self.elapsed_time, self.data_source) = struct.unpack("<HdIB", buffer)
        DataPacket.__init__(self, date, timestamp)

    def name(self):
        return "SystemTime"

    def __str__(self):
        rtn = DataPacket.__str__(self) + " " + self.name() + ": ellapsed = " + str(self.elapsed_time)\
              + " ms., source = " + str(self.data_source)
        return rtn


class Attitude(DataPacket):
    def __init__(self, buffer):
        (date, timestamp, self.yaw, self.pitch, self.roll) = struct.unpack("<Hdddd", buffer)
        DataPacket.__init__(self, date, timestamp)

    def name(self):
        return "Attitude"

    def __str__(self):
        rtn = DataPacket.__str__(self) + " " + self.name() + ": yaw = " + str(angle_to_degs(self.yaw))\
              + " deg, pitch = " + str(angle_to_degs(self.pitch))\
              + " deg, roll = " + str(angle_to_degs(self.roll)) + " deg"
        return rtn


class Depth(DataPacket):
    def __init__(self, buffer):
        (date, timestamp, self.depth, self.offset, self.range) = struct.unpack("<Hdddd", buffer)
        DataPacket.__init__(self, date, timestamp)

    def name(self):
        return "Depth"

    def __str__(self):
        rtn = DataPacket.__str__(self) + " " + self.name() + ": depth = " + str(self.depth) + "m, offset = "\
              + str(self.offset) + "m, range = " + str(self.range) + "m"
        return rtn


class COG(DataPacket):
    def __init__(self, buffer):
        (date, timestamp, self.courseOverGround, self.speedOverGround) = struct.unpack("<Hddd", buffer)
        DataPacket.__init__(self, date, timestamp)

    def name(self):
        return "Course Over Ground"

    def __str__(self):
        rtn = DataPacket.__str__(self) + " " + self.name() + ": course over ground = "\
              + str(angle_to_degs(self.courseOverGround)) + " deg, speed over ground = "\
              + str(self.speedOverGround) + " m/s"
        return rtn


class GNSS(DataPacket):
    def __init__(self, buffer):
        (date, timestamp, self.latitude, self.longitude, self.altitude, self.receiverType, self.receiverMethod,
         self.numSVs, self.horizontalDOP, self.positionDOP, self.separation, self.numRefStations, self.refStationType,
         self.refStationID, self.correctionAge) = struct.unpack("<HddddBBBdddBBHd", buffer)
        DataPacket.__init__(self, date, timestamp)

    def name(self):
        return "GNSS"

    def __str__(self):
        rtn = DataPacket.__str__(self) + " " + self.name() + ": latitude = " + str(self.latitude) + " deg, longitude = "\
              + str(self.longitude) + " deg, altitude = " + str(self.altitude) + " m, GNSS type = "\
              + str(self.receiverType) + ", GNSS method = " + str(self.receiverMethod) + ", num. SVs = "\
              + str(self.numSVs) + ", horizontal DOP = " + str(self.horizontalDOP) + ", position DOP = "\
              + str(self.positionDOP) + ", Geoid separation = " + str(self.separation) + "m, number of ref. stations = "\
              + str(self.numRefStations) + ", ref. station type = " + str(self.refStationType) + ", ref. station ID = "\
              + str(self.refStationID) + ", correction age = " + str(self.correctionAge)
        return rtn


class Environment(DataPacket):
    def __init__(self, buffer):
        (date, timestamp, self.tempSource, self.temperature, self.humiditySource, self.humidity, self.pressure) = struct.unpack("<HdBdBdd", buffer)
        DataPacket.__init__(self, date, timestamp)

    def name(self):
        return "Environment"

    def __str__(self):
        rtn = DataPacket.__str__(self) + " " + self.name() + ": temperature = " + str(temp_to_celsius(self.temperature)) + " ºC (source "\
              + str(self.tempSource) + "), humidity = " + str(self.humidity) + "% (source " + str(self.humiditySource) +\
              "), pressure = " + str(pressure_to_mbar(self.pressure)) + " mBar"
        return rtn


class Temperature(DataPacket):
    def __init__(self, buffer):
        (date, timestamp, self.tempSource, self.temperature) = struct.unpack("<HdBd", buffer)
        DataPacket.__init__(self, date, timestamp)

    def name(self):
        return "Temperature"

    def __str__(self):
        rtn = DataPacket.__str__(self) + " " + self.name() + ": temperature = " + str(temp_to_celsius(self.temperature))\
              + " ºC (source " + str(self.tempSource) + ")"
        return rtn


class Humidity(DataPacket):
    def __init__(self, buffer):
        (date, timestamp, self.humiditySource, self.humidity) = struct.unpack("<HdBd", buffer)
        DataPacket.__init__(self, date, timestamp)

    def name(self):
        return "Humidity"

    def __str__(self):
        rtn = DataPacket.__str__(self) + " " + self.name() + ": humidity = " + str(self.humidity) + " % (source "\
              + str(self.humiditySource) + ")"
        return rtn


class Pressure(DataPacket):
    def __init__(self, buffer):
        (date, timestamp, self.pressureSource, self.pressure) = struct.unpack("<HdBd", buffer)
        DataPacket.__init__(self, date, timestamp)

    def name(self):
        return "Pressure"

    def __str__(self):
        rtn = DataPacket.__str__(self) + " " + self.name() + ": pressure = " + str(pressure_to_mbar(self.pressure))\
              + " mBar (source " + str(self.pressureSource) + ")"
        return rtn


class SerialiserVersion(DataPacket):
    def __init__(self, buffer):
        (self.major, self.minor) = struct.unpack("<II", buffer)
        DataPacket.__init__(self, 0, 0.0)

    def name(self):
        return "SerialiserVersion"

    def __str__(self):
        rtn = DataPacket.__str__(self) + " " + self.name() + ": version = " + str(self.major) + "." + str(self.minor)
        return rtn


class PacketFactory:
    def __init__(self, file):
        self.file = file
        self.end_of_file = False

    def next_packet(self):
        if self.end_of_file:
            return None

        buffer = self.file.read(8)   # Header for each packet is U32 (ID) U32 (length in bytes)

        if len(buffer) < 8:
            print("Failed to read 8-byte packet header in PacketFactory")
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
        else:
            print("Unknown packet with ID " + pkt_id + " in input stream; ignored.")
            rtn = None

        return rtn

    def has_more(self):
        return not self.end_of_file
