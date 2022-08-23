import struct
import unittest
import math
import logging

import xmlrunner
import numpy as np

from wibl.core.logger_file import PacketTypes
from wibl.simulator.data import DataGenerator, State, FormattedAngle, format_angle, MAX_RAD
from wibl.simulator.data.writer import Writer, MemoryWriter


logger = logging.getLogger(__name__)


class TestDataGenerator(unittest.TestCase):
    """
    TODO: Add tests for the following packet types in `wibl.core.logger_file.py`:
        - COG
        - Environment
        - Temperature
        - Humidity
        - Pressure
        - SerialiserVersion
        - Motion
        - Metadata
        - AlgorithmRequest
        - JSONMetadata
        - NMEA0183Filter
    """
    def test_compute_checksum(self):
        self.assertEqual(86, DataGenerator.compute_checksum("$GPZDA,000000.000,01,01,2020,00,00*"))

    def test_format_angle(self):
        fa: FormattedAngle = format_angle(43.000003270800001)
        self.assertEqual(43, fa.degrees)
        self.assertEqual(0.0000032708000006209659, fa.minutes)
        self.assertEqual(1, fa.hemisphere)

        fa: FormattedAngle = format_angle(-74.999996729200006)
        self.assertEqual(74, fa.degrees)
        self.assertEqual(0.99999672920000648, fa.minutes)
        self.assertEqual(0, fa.hemisphere)

    def test_unit_normal(self):
        state: State = State()

        max_itr: int = 1_000_000
        test_unit_normal = np.zeros((max_itr,))
        for i in range(max_itr):
            test_unit_normal[i] = state.unit_normal()

        logger.debug(f"Num iterations: {max_itr}")
        logger.debug(f"Mean: {np.mean(test_unit_normal)}, min: {min(test_unit_normal)}, max: {max(test_unit_normal)}")

        self.assertAlmostEqual(0.0, np.mean(test_unit_normal), 2)
        self.assertLess(-5.75, min(test_unit_normal))
        self.assertGreater(5.75, max(test_unit_normal))

    def __test_header_packets(self, buff: bytes):
        # First packet is SerialiserVersion
        # Packet ID for SerialiserVersion packet type is 0
        self.assertEqual(0, struct.unpack('<I', buff[0:4])[0])
        # Length for SerialiserVersion packet type is 16
        self.assertEqual(16, struct.unpack('<I', buff[4:8])[0])
        # Major version in bytes 7-8, equals 1
        self.assertEqual(1, struct.unpack('<H', buff[8:10])[0])
        # Minor version in bytes 9-10, equals 0
        self.assertEqual(0, struct.unpack('<H', buff[10:12])[0])
        # NMEA2000 major version in bytes 11-12, equals 1
        self.assertEqual(1, struct.unpack('<H', buff[12:14])[0])
        # NMEA2000 minor version in bytes 13-14, equals 0
        self.assertEqual(0, struct.unpack('<H', buff[14:16])[0])
        # NMEA2000 patch version in bytes 15-16, equals 0
        self.assertEqual(0, struct.unpack('<H', buff[16:18])[0])
        # NMEA0183 major version in bytes 17-18, equals 1
        self.assertEqual(1, struct.unpack('<H', buff[18:20])[0])
        # NMEA0183 minor version in bytes 19-20, equals 0
        self.assertEqual(0, struct.unpack('<H', buff[20:22])[0])
        # NMEA0183 patch version in bytes 21-22, equals 0
        self.assertEqual(0, struct.unpack('<H', buff[22:24])[0])

        # Second packet is Metadata
        # Packet ID for Metadata packet type is 12
        self.assertEqual(12, struct.unpack('<I', buff[24:28])[0])
        # Length for this Metadata packet is 35
        self.assertEqual(35, struct.unpack('<I', buff[28:32])[0])
        # Logger name length in bytes 31-34, equals 13
        self.assertEqual(13, struct.unpack('<I', buff[32:36])[0])
        # Logger name in bytes 35-47, equals 'Gulf Surveyor'
        self.assertEqual('Gulf Surveyor', struct.unpack('<13s', buff[36:49])[0].decode('UTF-8'))
        # Logger ID length in bytes 48-51, equals 14
        self.assertEqual(14, struct.unpack('<I', buff[49:53])[0])
        # Logger ID in bytes 52-65, equals 'WIBL-Simulator'
        self.assertEqual('WIBL-Simulator', struct.unpack('<14s', buff[53:67])[0].decode('UTF-8'))

    def test_generate_gga(self):
        state: State = State()
        # Simulate first position after initial position step
        state.current_longitude = -74.999996729200006
        state.current_latitude = 43.000003270800001
        # Simulate first time after initial time step
        state.update_ticks(300536)
        state.ref_time.update(state.curr_ticks)
        state.sim_time.update(math.floor(state.curr_ticks / 2))
        gen: DataGenerator = DataGenerator(use_data_constructor=True)

        writer: Writer = MemoryWriter('Gulf Surveyor', 'WIBL-Simulator')
        gen.generate_gga(state, writer)

        buff: bytes = writer.getvalue()
        self.assertIsNotNone(buff)
        self.__test_header_packets(buff)

        # Message type in bytes 66-69
        self.assertEqual(PacketTypes.SerialString.value, struct.unpack('<I', buff[67:71])[0])
        # Message length in bytes 70-73
        self.assertEqual(88, struct.unpack('<I', buff[71:75])[0])
        # Days since epoch from bytes 74-77
        self.assertEqual(state.sim_time.tick_count_to_milliseconds(), struct.unpack('<I', buff[75:79])[0])

        # First byte of message body is '$' = ASCII 36 (decimal)
        self.assertEqual(36, buff[79])
        # Message descriptor: GPGGA
        # G = 71
        self.assertEqual(71, buff[80])
        # P = 80
        self.assertEqual(80, buff[81])
        self.assertEqual(71, buff[82])
        self.assertEqual(71, buff[83])
        # A = 65
        self.assertEqual(65, buff[84])
        # Component separator is , = 44
        self.assertEqual(44, buff[85])
        # Hour, minute, and decimal seconds (all initially ASCII 0=48 decimal)
        self.assertEqual(48, buff[86])
        self.assertEqual(48, buff[87])
        self.assertEqual(48, buff[88])
        self.assertEqual(48, buff[89])
        # Seconds '00.500'
        self.assertEqual(48, buff[90])
        # '0' = 48
        self.assertEqual(48, buff[91])
        # '.' = 46
        self.assertEqual(46, buff[92])
        # Decimal part of seconds
        # '1' = 49
        self.assertEqual(49, buff[93])
        # '5' = 53
        self.assertEqual(53, buff[94])
        # '0' = 48
        self.assertEqual(48, buff[95])
        # Component separator is , = 44
        self.assertEqual(44, buff[96])
        # Beginning of latitude: 43 degrees, 00.000003 minutes/seconds (in ASCII)
        self.assertEqual(52, buff[97])
        self.assertEqual(51, buff[98])
        self.assertEqual(48, buff[99])
        self.assertEqual(48, buff[100])
        # . = 46
        self.assertEqual(46, buff[101])
        self.assertEqual(48, buff[102])
        self.assertEqual(48, buff[103])
        self.assertEqual(48, buff[104])
        self.assertEqual(48, buff[105])
        self.assertEqual(48, buff[106])
        # 3 = 51
        self.assertEqual(51, buff[107])
        # Component separator is , = 44
        self.assertEqual(44, buff[108])
        # N (North) = 78
        self.assertEqual(78, buff[109])
        # Component separator is , = 44 (end of latitude)
        self.assertEqual(44, buff[110])
        # Beginning of longitude: 074 degrees, 00.999997 minutes/seconds (in ASCII)
        self.assertEqual(48, buff[111])
        # 7 = 55
        self.assertEqual(55, buff[112])
        # 4 = 52
        self.assertEqual(52, buff[113])
        self.assertEqual(48, buff[114])
        self.assertEqual(48, buff[115])
        # . = 46
        self.assertEqual(46, buff[116])
        # 9 = 57
        self.assertEqual(57, buff[117])
        self.assertEqual(57, buff[118])
        self.assertEqual(57, buff[119])
        self.assertEqual(57, buff[120])
        self.assertEqual(57, buff[121])
        # 7 = 55
        self.assertEqual(55, buff[122])
        # Component separator is , = 44
        self.assertEqual(44, buff[123])
        # W (West) = 87
        self.assertEqual(87, buff[124])
        # Component separator is , = 44
        self.assertEqual(44, buff[125])
        # Dummy part of message: 3,12,1.0,-19.5,M,22.5,M,0.0,0000*
        # '3'
        self.assertEqual(51, buff[126])
        # Component separator is , = 44
        self.assertEqual(44, buff[127])
        # '12'
        self.assertEqual(49, buff[128])
        self.assertEqual(50, buff[129])
        # Component separator is , = 44
        self.assertEqual(44, buff[130])
        # '1.0'
        self.assertEqual(49, buff[131])
        self.assertEqual(46, buff[132])
        self.assertEqual(48, buff[133])
        # Component separator is , = 44
        self.assertEqual(44, buff[134])
        # '-19.5'
        self.assertEqual(45, buff[135])
        self.assertEqual(49, buff[136])
        self.assertEqual(57, buff[137])
        self.assertEqual(46, buff[138])
        self.assertEqual(53, buff[139])
        # Component separator is , = 44
        self.assertEqual(44, buff[140])
        # 'M'
        self.assertEqual(77, buff[141])
        # Component separator is , = 44
        self.assertEqual(44, buff[142])
        # '22.5'
        self.assertEqual(50, buff[143])
        self.assertEqual(50, buff[144])
        self.assertEqual(46, buff[145])
        self.assertEqual(53, buff[146])
        # Component separator is , = 44
        self.assertEqual(44, buff[147])
        # 'M'
        self.assertEqual(77, buff[148])
        # Component separator is , = 44
        self.assertEqual(44, buff[149])
        # '0.0'
        self.assertEqual(48, buff[150])
        self.assertEqual(46, buff[151])
        self.assertEqual(48, buff[152])
        # Component separator is , = 44
        self.assertEqual(44, buff[153])
        # '0000*'
        self.assertEqual(48, buff[154])
        self.assertEqual(48, buff[155])
        self.assertEqual(48, buff[156])
        self.assertEqual(48, buff[157])
        self.assertEqual(42, buff[158])
        # Checksum: '78'
        self.assertEqual(b'78', buff[159:161])
        # End of message: '\r\n'
        self.assertEqual(b'\r\n', buff[161:163])

        with self.assertRaises(IndexError):
            buff[163]

    def test_generate_gga_compare(self):
        """
        Compare results from buffer vs. data constructor for GGA SerialString packet
        :return:
        """
        state: State = State()
        # Simulate first position after initial position step
        state.current_longitude = -74.999996729200006
        state.current_latitude = 43.000003270800001
        # Simulate first time after initial time step
        state.update_ticks(300536)
        state.ref_time.update(state.curr_ticks)
        state.sim_time.update(math.floor(state.curr_ticks / 2))

        gen_data_const: DataGenerator = DataGenerator(use_data_constructor=True)
        writer_data_const: Writer = MemoryWriter('Gulf Surveyor', 'WIBL-Simulator')
        gen_data_const.generate_gga(state, writer_data_const)
        buff_data_const: bytes = writer_data_const.getvalue()

        gen_buff_const: DataGenerator = DataGenerator(use_data_constructor=False)
        writer_buff_const: Writer = MemoryWriter('Gulf Surveyor', 'WIBL-Simulator')
        gen_buff_const.generate_gga(state, writer_buff_const)
        buff_buff_const: bytes = writer_buff_const.getvalue()

        self.assertEqual(buff_data_const, buff_buff_const)

    def test_generate_system_time(self):
        state: State = State()
        # Simulate first time after initial time step
        state.update_ticks(300536)
        state.ref_time.update(state.curr_ticks)
        gen: DataGenerator = DataGenerator(use_data_constructor=True)

        writer: Writer = MemoryWriter('Gulf Surveyor', 'WIBL-Simulator')
        gen.generate_system_time(state, writer)

        buff: bytes = writer.getvalue()
        self.assertIsNotNone(buff)
        self.__test_header_packets(buff)

        # Message type in bytes 66-69
        self.assertEqual(PacketTypes.SystemTime.value, struct.unpack('<I', buff[67:71])[0])
        # Message length in bytes 70-73
        self.assertEqual(15, struct.unpack('<I', buff[71:75])[0])
        # Days since epoch from bytes 74-75
        self.assertEqual(18262, struct.unpack('<H', buff[75:77])[0])
        # Read timestamp from bytes 76-83
        self.assertEqual(0.300536, struct.unpack('<d', buff[77:85])[0])
        # Elapsed time should equal state.tick_count_to_milliseconds()
        self.assertEqual(state.tick_count_to_milliseconds(), struct.unpack('<I', buff[85:89])[0])
        # Data source is the final byte (23) and should be 0 for now
        self.assertEqual(0, buff[89])

        with self.assertRaises(IndexError):
            buff[90]

    def test_generate_system_time_compare(self):
        """
        Compare results from buffer vs. data constructor for SystemTime packet
        :return:
        """
        state: State = State()
        # Simulate first time after initial time step
        state.update_ticks(300536)
        state.ref_time.update(state.curr_ticks)

        gen_data_const: DataGenerator = DataGenerator(use_data_constructor=True)
        writer_data_const: Writer = MemoryWriter('Gulf Surveyor', 'WIBL-Simulator')
        gen_data_const.generate_system_time(state, writer_data_const)
        buff_data_const: bytes = writer_data_const.getvalue()

        gen_buffer_const: DataGenerator = DataGenerator(use_data_constructor=False)
        writer_buffer_const: Writer = MemoryWriter('Gulf Surveyor', 'WIBL-Simulator')
        gen_buffer_const.generate_system_time(state, writer_buffer_const)
        buff_buffer_const: bytes = writer_buffer_const.getvalue()

        self.assertEqual(buff_data_const, buff_buffer_const)

    def test_generate_attitude(self):
        state: State = State()
        # Simulate first time after initial time step
        state.update_ticks(300536)
        state.ref_time.update(state.curr_ticks)
        gen: DataGenerator = DataGenerator(use_data_constructor=True)

        writer: Writer = MemoryWriter('Gulf Surveyor', 'WIBL-Simulator')
        gen.generate_attitude(state, writer)

        buff: bytes = writer.getvalue()
        self.assertIsNotNone(buff)
        self.__test_header_packets(buff)

        # Message type in bytes 66-69
        self.assertEqual(PacketTypes.Attitude.value, struct.unpack('<I', buff[67:71])[0])
        # Message length in bytes 70-73
        self.assertEqual(38, struct.unpack('<I', buff[71:75])[0])
        # Days since epoch from bytes 74-75
        self.assertEqual(18262, struct.unpack('<H', buff[75:77])[0])
        # Read timestamp from bytes 76-83
        self.assertEqual(0.300536, struct.unpack('<d', buff[77:85])[0])
        # Elapsed time should equal state.tick_count_to_milliseconds()
        self.assertEqual(state.tick_count_to_milliseconds(), struct.unpack('<I', buff[85:89])[0])
        # Read yaw from bytes 88-95
        yaw = struct.unpack('<d', buff[89:97])[0]
        self.assertGreaterEqual(yaw, 0)
        self.assertLessEqual(yaw, MAX_RAD)
        # Read pitch from bytes 96-103
        pitch = struct.unpack('<d', buff[97:105])[0]
        self.assertGreaterEqual(pitch, 0)
        self.assertLessEqual(pitch, MAX_RAD)
        # Read roll from bytes 104-111
        roll = struct.unpack('<d', buff[105:113])[0]
        self.assertGreaterEqual(roll, 0)
        self.assertLessEqual(roll, MAX_RAD)

        with self.assertRaises(IndexError):
            buff[114]

    def test_generate_attitude_compare(self):
        """
        Compare results from buffer vs. data constructor for Attitude packet
        :return:
        """
        state: State = State()
        # Simulate first time after initial time step
        state.update_ticks(300536)
        state.ref_time.update(state.curr_ticks)

        gen_data_const: DataGenerator = DataGenerator(use_data_constructor=True)
        writer_data_const: Writer = MemoryWriter('Gulf Surveyor', 'WIBL-Simulator')
        gen_data_const.generate_attitude(state, writer_data_const)
        buff_data_const: bytes = writer_data_const.getvalue()

        gen_buffer_const: DataGenerator = DataGenerator(use_data_constructor=False)
        writer_buffer_const: Writer = MemoryWriter('Gulf Surveyor', 'WIBL-Simulator')
        gen_buffer_const.generate_attitude(state, writer_buffer_const)
        buff_buffer_const: bytes = writer_buffer_const.getvalue()

        self.assertEqual(buff_data_const, buff_buffer_const)


    def test_generate_gnss(self):
        state: State = State()
        # Simulate first time after initial time step
        state.update_ticks(300536)
        state.ref_time.update(state.curr_ticks)
        state.sim_time.update(math.floor(state.curr_ticks / 2))
        # Simulate first position after initial position step
        state.current_longitude = -74.999996729200006
        state.current_latitude = 43.000003270800001
        gen: DataGenerator = DataGenerator(use_data_constructor=True)

        writer: Writer = MemoryWriter('Gulf Surveyor', 'WIBL-Simulator')
        gen.generate_gnss(state, writer)

        buff: bytes = writer.getvalue()
        self.assertIsNotNone(buff)
        self.__test_header_packets(buff)

        # Message type in bytes 66-69
        self.assertEqual(PacketTypes.GNSS.value, struct.unpack('<I', buff[67:71])[0])
        # Message length in bytes 70-73
        self.assertEqual(87, struct.unpack('<I', buff[71:75])[0])
        # Days since epoch from bytes 74-75
        self.assertEqual(18262, struct.unpack('<H', buff[75:77])[0])
        # Read timestamp from bytes 76-83
        self.assertEqual(0.150268, struct.unpack('<d', buff[77:85])[0])
        # Elapsed time should equal state.tick_count_to_milliseconds()
        self.assertEqual(state.sim_time.tick_count_to_milliseconds(), struct.unpack('<I', buff[85:89])[0])

        # Message date should equal state.sim_time.days_since_epoch()
        self.assertEqual(state.sim_time.days_since_epoch(), struct.unpack('<H', buff[89:91])[0])
        # Message timestamp = state.sim_time.seconds_in_day()
        self.assertEqual(state.sim_time.seconds_in_day(), struct.unpack('<d', buff[91:99])[0])
        # Latitude
        self.assertEqual(state.current_latitude, struct.unpack('<d', buff[99:107])[0])
        # Longitude
        self.assertEqual(state.current_longitude, struct.unpack('<d', buff[107:115])[0])
        # Hard-coded altitude
        self.assertEqual(-19.323, struct.unpack('<d', buff[115:123])[0])
        # Hard-coded rx_type
        self.assertEqual(0, buff[123])
        # Hard-coded rx_method
        self.assertEqual(2, buff[124])
        # Hard-coded num SVs
        self.assertEqual(12, buff[125])
        # Hard-coded horizontal DOP
        self.assertEqual(1.5, struct.unpack('<d', buff[126:134])[0])
        # Hard-coded position DOP
        self.assertEqual(2.2, struct.unpack('<d', buff[134:142])[0])
        # Hard-coded sep
        self.assertEqual(22.3453, struct.unpack('<d', buff[142:150])[0])
        # Hard-coded n_refs
        self.assertEqual(1, buff[150])
        # Hard-coded refs_type
        self.assertEqual(4, buff[151])
        # Hard-coded refs_id
        self.assertEqual(12312, struct.unpack('<H', buff[152:154])[0])
        # Hard-coded correction_age
        self.assertEqual(2.32, struct.unpack('<d', buff[154:162])[0])

        with self.assertRaises(IndexError):
            buff[162]

    def test_generate_gnss_compare(self):
        """
        Compare results from buffer vs. data constructor for GNSS packet
        :return:
        """
        state: State = State()
        # Simulate first time after initial time step
        state.update_ticks(300536)
        state.ref_time.update(state.curr_ticks)
        state.sim_time.update(math.floor(state.curr_ticks / 2))
        # Simulate first position after initial position step
        state.current_longitude = -74.999996729200006
        state.current_latitude = 43.000003270800001

        gen_data_const: DataGenerator = DataGenerator(use_data_constructor=True)
        writer_data_const: Writer = MemoryWriter('Gulf Surveyor', 'WIBL-Simulator')
        gen_data_const.generate_gnss(state, writer_data_const)
        buff_data_const: bytes = writer_data_const.getvalue()

        gen_buff_const: DataGenerator = DataGenerator(use_data_constructor=False)
        writer_buff_const: Writer = MemoryWriter('Gulf Surveyor', 'WIBL-Simulator')
        gen_buff_const.generate_gnss(state, writer_buff_const)
        buff_buff_const: bytes = writer_buff_const.getvalue()

        self.assertEqual(buff_data_const, buff_buff_const)

    def test_generate_depth(self):
        state: State = State()
        # Simulate first time after initial time step
        state.update_ticks(300536)
        state.ref_time.update(state.curr_ticks)
        gen: DataGenerator = DataGenerator(use_data_constructor=True)

        writer: Writer = MemoryWriter('Gulf Surveyor', 'WIBL-Simulator')
        gen.generate_depth(state, writer)

        buff: bytes = writer.getvalue()
        self.assertIsNotNone(buff)
        self.__test_header_packets(buff)

        # Message type in bytes 66-69
        self.assertEqual(PacketTypes.Depth.value, struct.unpack('<I', buff[67:71])[0])
        # Message length in bytes 70-73
        self.assertEqual(38, struct.unpack('<I', buff[71:75])[0])
        # Days since epoch from bytes 74-75
        self.assertEqual(18262, struct.unpack('<H', buff[75:77])[0])
        # Read timestamp from bytes 76-83
        self.assertEqual(0.0, struct.unpack('<d', buff[77:85])[0])
        # Elapsed time should equal state.tick_count_to_milliseconds()
        self.assertEqual(state.sim_time.tick_count_to_milliseconds(), struct.unpack('<I', buff[85:89])[0])

        # Depth should equal state.current_depth
        self.assertEqual(state.current_depth, struct.unpack('<d', buff[89:97])[0])
        # Hard-coded offset
        self.assertEqual(0.0, struct.unpack('<d', buff[97:105])[0])
        # Hard-coded range
        self.assertEqual(200.0, struct.unpack('<d', buff[105:113])[0])

        with self.assertRaises(IndexError):
            buff[113]

    def test_generate_depth_compare(self):
        """
        Compare results from buffer vs. data constructor for Depth packet
        :return:
        """
        state: State = State()
        # Simulate first time after initial time step
        state.update_ticks(300536)
        state.ref_time.update(state.curr_ticks)

        gen_data_const: DataGenerator = DataGenerator(use_data_constructor=True)
        writer_data_const: Writer = MemoryWriter('Gulf Surveyor', 'WIBL-Simulator')
        gen_data_const.generate_depth(state, writer_data_const)
        buff_data_const: bytes = writer_data_const.getvalue()

        gen_buff_const: DataGenerator = DataGenerator(use_data_constructor=False)
        writer_buff_const: Writer = MemoryWriter('Gulf Surveyor', 'WIBL-Simulator')
        gen_buff_const.generate_depth(state, writer_buff_const)
        buff_buff_const: bytes = writer_data_const.getvalue()

        self.assertEqual(buff_data_const, buff_buff_const)

    def test_generate_zda(self):
        state: State = State()
        # Simulate first position after initial position step
        state.current_longitude = -74.999996729200006
        state.current_latitude = 43.000003270800001
        # Simulate first time after initial time step
        state.update_ticks(300536)
        state.ref_time.update(state.curr_ticks)
        state.sim_time.update(math.floor(state.curr_ticks / 2))
        gen: DataGenerator = DataGenerator(use_data_constructor=True)

        writer: Writer = MemoryWriter('Gulf Surveyor', 'WIBL-Simulator')
        gen.generate_zda(state, writer)

        buff: bytes = writer.getvalue()
        self.assertIsNotNone(buff)
        self.__test_header_packets(buff)

        # Message type in bytes 66-69
        self.assertEqual(PacketTypes.SerialString.value, struct.unpack('<I', buff[67:71])[0])
        # Message length in bytes 70-73
        self.assertEqual(43, struct.unpack('<I', buff[71:75])[0])
        # Days since epoch from bytes 74-77
        self.assertEqual(state.sim_time.tick_count_to_milliseconds(), struct.unpack('<I', buff[75:79])[0])

        # First byte of message body is '$' = ASCII 36 (decimal)
        self.assertEqual(36, buff[79])
        # Message descriptor: GPZDA
        # G = 71
        self.assertEqual(71, buff[80])
        # P = 80
        self.assertEqual(80, buff[81])
        # Z = 90
        self.assertEqual(90, buff[82])
        # D = 68
        self.assertEqual(68, buff[83])
        # A = 65
        self.assertEqual(65, buff[84])
        # Component separator is , = 44
        self.assertEqual(44, buff[85])
        # Hour, minute, and decimal seconds (all initially ASCII 0=48 decimal)
        self.assertEqual(48, buff[86])
        self.assertEqual(48, buff[87])
        self.assertEqual(48, buff[88])
        self.assertEqual(48, buff[89])
        # Seconds '00.150'
        self.assertEqual(48, buff[90])
        # '0' = 48
        self.assertEqual(48, buff[91])
        # '.' = 46
        self.assertEqual(46, buff[92])
        # Decimal part of seconds
        # '1' = 49
        self.assertEqual(49, buff[93])
        # '5' = 53
        self.assertEqual(53, buff[94])
        # '0' = 48
        self.assertEqual(48, buff[95])
        # Component separator is , = 44
        self.assertEqual(44, buff[96])
        # Month day is '01'
        self.assertEqual(b'01', buff[97:99])
        # Component separator is , = 44
        self.assertEqual(44, buff[99])
        # Month is '01'
        self.assertEqual(b'01', buff[100:102])
        # Component separator is , = 44
        self.assertEqual(44, buff[102])
        # Year is '2020'
        self.assertEqual(b'2020', buff[103:107])
        # Component separator is , = 44
        self.assertEqual(44, buff[107])
        # End of message is hard-coded with '00,00*'
        self.assertEqual(b'00,00*', buff[108:114])
        # Checksum: '52'
        self.assertEqual(b'52', buff[114:116])
        # End of message: '\r\n'
        self.assertEqual(b'\r\n', buff[116:118])

        with self.assertRaises(IndexError):
            buff[118]

    def test_generate_zda_compare(self):
        """
        Compare results from buffer vs. data constructor for ZDA SerialString packet
        :return:
        """
        state: State = State()
        # Simulate first position after initial position step
        state.current_longitude = -74.999996729200006
        state.current_latitude = 43.000003270800001
        # Simulate first time after initial time step
        state.update_ticks(300536)
        state.ref_time.update(state.curr_ticks)
        state.sim_time.update(math.floor(state.curr_ticks / 2))

        gen_data_const: DataGenerator = DataGenerator(use_data_constructor=True)
        writer_data_const: Writer = MemoryWriter('Gulf Surveyor', 'WIBL-Simulator')
        gen_data_const.generate_zda(state, writer_data_const)
        buff_data_const: bytes = writer_data_const.getvalue()

        gen_buff_const: DataGenerator = DataGenerator(use_data_constructor=False)
        writer_buff_const: Writer = MemoryWriter('Gulf Surveyor', 'WIBL-Simulator')
        gen_buff_const.generate_zda(state, writer_buff_const)
        buff_buff_const: bytes = writer_buff_const.getvalue()

        self.assertEqual(buff_data_const, buff_buff_const)

    def test_generate_dbt(self):
        state: State = State()
        # Simulate first position after initial position step
        state.current_longitude = -74.999996729200006
        state.current_latitude = 43.000003270800001
        # Simulate first time after initial time step
        state.update_ticks(300536)
        state.ref_time.update(state.curr_ticks)
        state.sim_time.update(math.floor(state.curr_ticks / 2))
        gen: DataGenerator = DataGenerator(use_data_constructor=True)

        writer: Writer = MemoryWriter('Gulf Surveyor', 'WIBL-Simulator')
        gen.generate_dbt(state, writer, override_depth=10.0)

        buff: bytes = writer.getvalue()
        self.assertIsNotNone(buff)
        self.__test_header_packets(buff)

        # Message type in bytes 66-69
        self.assertEqual(PacketTypes.SerialString.value, struct.unpack('<I', buff[67:71])[0])
        # Message length in bytes 70-73
        # Length may vary a bit due to random depth values, so let's just make sure it's a valid int
        exc_raised = False
        try:
            msg_len = int(struct.unpack('<I', buff[71:75])[0])
        except ValueError:
            exc_raised = True
        self.assertFalse(exc_raised)
        # Days since epoch from bytes 74-77
        self.assertEqual(state.sim_time.tick_count_to_milliseconds(), struct.unpack('<I', buff[75:79])[0])

        # First byte of message body is '$' = ASCII 36 (decimal)
        self.assertEqual(36, buff[79])
        # Message descriptor: SDDBT
        # S = 83
        self.assertEqual(83, buff[80])
        # D = 68
        self.assertEqual(68, buff[81])
        # D = 68
        self.assertEqual(68, buff[82])
        # B = 66
        self.assertEqual(66, buff[83])
        # T = 84
        self.assertEqual(84, buff[84])
        # Component separator is , = 44
        self.assertEqual(44, buff[85])

        # Depth in feet
        self.assertEqual(32.8, float(buff[86:90]))
        # Component separator is , = 44
        self.assertEqual(44, buff[90])
        # Unit: 'f' = 102
        self.assertEqual(102, buff[91])
        # Component separator is , = 44
        self.assertEqual(44, buff[92])

        # Depth in metres
        exc_raised = False
        self.assertEqual(10.0, float(buff[93:97]))
        # Component separator is , = 44
        self.assertEqual(44, buff[97])
        # Unit: 'M' = 77
        self.assertEqual(77, buff[98])
        # Component separator is , = 44
        self.assertEqual(44, buff[99])

        # Depth in fathoms
        self.assertEqual(5.5, float(buff[100:103]))
        # Component separator is , = 44
        self.assertEqual(44, buff[103])
        # Unit and message end: 'F*'
        self.assertEqual(b'F*', buff[104:106])

        # Checksum
        self.assertEqual(14, int(buff[106:108], 16))
        # End of message: '\r\n'
        self.assertEqual(b'\r\n', buff[108:110])

        with self.assertRaises(IndexError):
            buff[110]

    def test_generate_dbt_compare(self):
        """
        Compare results from buffer vs. data constructor for DBT SerialString packet
        :return:
        """
        state: State = State()
        # Simulate first position after initial position step
        state.current_longitude = -74.999996729200006
        state.current_latitude = 43.000003270800001
        # Simulate first time after initial time step
        state.update_ticks(300536)
        state.ref_time.update(state.curr_ticks)
        state.sim_time.update(math.floor(state.curr_ticks / 2))

        gen_data_const: DataGenerator = DataGenerator(use_data_constructor=True)
        writer_data_const: Writer = MemoryWriter('Gulf Surveyor', 'WIBL-Simulator')
        gen_data_const.generate_dbt(state, writer_data_const, override_depth=10.0)
        buff_data_const: bytes = writer_data_const.getvalue()

        gen_buff_const: DataGenerator = DataGenerator(use_data_constructor=False)
        writer_buff_const: Writer = MemoryWriter('Gulf Surveyor', 'WIBL-Simulator')
        gen_buff_const.generate_dbt(state, writer_buff_const, override_depth=10.0)
        buff_buff_const: bytes = writer_buff_const.getvalue()

        self.assertEqual(buff_data_const, buff_buff_const)


if __name__ == '__main__':
    unittest.main(
        testRunner=xmlrunner.XMLTestRunner(output='test-reports'),
        failfast=False, buffer=False, catchbreak=False
    )
