import struct
import unittest
import io
import math

from wibl.core.logger_file import PacketTypes
from wibl.simulator.data import DataGenerator, State, FormattedAngle, format_angle


class TestDataGenerator(unittest.TestCase):
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

    def test_generate_gga(self):
        state: State = State()
        # Simulate first position after initial position step
        state.current_longitude = -74.999996729200006
        state.current_latitude = 43.000003270800001
        # Simulate first time after initial time step
        state.update_ticks(300536)
        state.ref_time.update(state.curr_ticks)
        state.sim_time.update(math.floor(state.curr_ticks / 2))
        gen: DataGenerator = DataGenerator()

        bio = io.BytesIO()
        writer = io.BufferedWriter(bio)
        gen.generate_gga(state, writer)

        writer.flush()
        buff: bytes = bio.getvalue()
        self.assertIsNotNone(buff)

        # Message type is the 1st-4th bytes
        self.assertEqual(PacketTypes.SerialString.value, struct.unpack('<I', buff[0:4])[0])
        # Message length is the 5th-9th bytes
        self.assertEqual(89, struct.unpack('<I', buff[4:8])[0])
        # Days since epoch from bytes 9 and 10
        self.assertEqual(state.tick_count, struct.unpack('<I', buff[8:12])[0])

        # First byte of message body is '$' = ASCII 36 (decimal)
        self.assertEqual(36, buff[12])
        # Message descriptor: GPGGA
        # G = 71
        self.assertEqual(71, buff[13])
        # P = 80
        self.assertEqual(80, buff[14])
        self.assertEqual(71, buff[15])
        self.assertEqual(71, buff[16])
        # A = 65
        self.assertEqual(65, buff[17])
        # Component separator is , = 44
        self.assertEqual(44, buff[18])
        # Hour, minute, and decimal seconds (all initially ASCII 0=48 decimal)
        self.assertEqual(48, buff[19])
        self.assertEqual(48, buff[20])
        self.assertEqual(48, buff[21])
        self.assertEqual(48, buff[22])
        # Seconds '01.503'
        self.assertEqual(48, buff[23])
        # '1' = 49
        self.assertEqual(49, buff[24])
        # '.' = 46
        self.assertEqual(46, buff[25])
        # Decimal part of seconds
        # '5' = 53
        self.assertEqual(53, buff[26])
        self.assertEqual(48, buff[27])
        # '3' = 51
        self.assertEqual(51, buff[28])
        # Component separator is , = 44
        self.assertEqual(44, buff[29])
        # Beginning of latitude: 43 degrees, 00.000003 minutes/seconds (in ASCII)
        self.assertEqual(52, buff[30])
        self.assertEqual(51, buff[31])
        self.assertEqual(48, buff[32])
        self.assertEqual(48, buff[33])
        # . = 46
        self.assertEqual(46, buff[34])
        self.assertEqual(48, buff[35])
        self.assertEqual(48, buff[36])
        self.assertEqual(48, buff[37])
        self.assertEqual(48, buff[38])
        self.assertEqual(48, buff[39])
        # 3 = 51
        self.assertEqual(51, buff[40])
        # Component separator is , = 44
        self.assertEqual(44, buff[41])
        # N (North) = 78
        self.assertEqual(78, buff[42])
        # Component separator is , = 44 (end of latitude)
        self.assertEqual(44, buff[43])
        # Beginning of longitude: 074 degrees, 00.999997 minutes/seconds (in ASCII)
        self.assertEqual(48, buff[44])
        # 7 = 55
        self.assertEqual(55, buff[45])
        # 4 = 52
        self.assertEqual(52, buff[46])
        self.assertEqual(48, buff[47])
        self.assertEqual(48, buff[48])
        # . = 46
        self.assertEqual(46, buff[49])
        # 9 = 57
        self.assertEqual(57, buff[50])
        self.assertEqual(57, buff[51])
        self.assertEqual(57, buff[52])
        self.assertEqual(57, buff[53])
        self.assertEqual(57, buff[54])
        # 7 = 55
        self.assertEqual(55, buff[55])
        # Component separator is , = 44
        self.assertEqual(44, buff[56])
        # W (West) = 87
        self.assertEqual(87, buff[57])
        # Component separator is , = 44
        self.assertEqual(44, buff[58])
        # Dummy party of message: 3,12,1.0,-19.5,M,22.5,M,0.0,0000*
        # '3'
        self.assertEqual(51, buff[59])
        # Component separator is , = 44
        self.assertEqual(44, buff[60])
        # '12'
        self.assertEqual(49, buff[61])
        self.assertEqual(50, buff[62])
        # Component separator is , = 44
        self.assertEqual(44, buff[63])
        # '1.0'
        self.assertEqual(49, buff[64])
        self.assertEqual(46, buff[65])
        self.assertEqual(48, buff[66])
        # Component separator is , = 44
        self.assertEqual(44, buff[67])
        # '-19.5'
        self.assertEqual(45, buff[68])
        self.assertEqual(49, buff[69])
        self.assertEqual(57, buff[70])
        self.assertEqual(46, buff[71])
        self.assertEqual(53, buff[72])
        # Component separator is , = 44
        self.assertEqual(44, buff[73])
        # 'M'
        self.assertEqual(77, buff[74])
        # Component separator is , = 44
        self.assertEqual(44, buff[75])
        # '22.5'
        self.assertEqual(50, buff[76])
        self.assertEqual(50, buff[77])
        self.assertEqual(46, buff[78])
        self.assertEqual(53, buff[79])
        # Component separator is , = 44
        self.assertEqual(44, buff[80])
        # 'M'
        self.assertEqual(77, buff[81])
        # Component separator is , = 44
        self.assertEqual(44, buff[82])
        # '0.0'
        self.assertEqual(48, buff[83])
        self.assertEqual(46, buff[84])
        self.assertEqual(48, buff[85])
        # Component separator is , = 44
        self.assertEqual(44, buff[86])
        # '0000*'
        self.assertEqual(48, buff[87])
        self.assertEqual(48, buff[88])
        self.assertEqual(48, buff[89])
        self.assertEqual(48, buff[90])
        self.assertEqual(42, buff[91])
        # Component separator is , = 44
        self.assertEqual(44, buff[92])
        # Checksum: '7C'
        self.assertEqual(b'7B', buff[93:95])
        # End of message: '\r\n'
        self.assertEqual(b'\r\n', buff[95:97])

    def test_generate_system_time(self):
        state: State = State()
        # Simulate first time after initial time step
        state.update_ticks(300536)
        state.ref_time.update(state.curr_ticks)
        gen: DataGenerator = DataGenerator()

        bio = io.BytesIO()
        writer = io.BufferedWriter(bio)
        gen.generate_system_time(state, writer)

        writer.flush()
        buff: bytes = bio.getvalue()
        self.assertIsNotNone(buff)
        # Message type is the 1st-4th bytes
        self.assertEqual(PacketTypes.SystemTime.value, struct.unpack('<I', buff[0:4])[0])
        # Message length is the 5th-9th bytes
        self.assertEqual(15, struct.unpack('<I', buff[4:8])[0])
        # Days since epoch from bytes 9 and 10
        self.assertEqual(18262, struct.unpack('<H', buff[8:10])[0])
        # Read timestamp from bytes 11-19
        timestamp = struct.unpack('<d', buff[10:18])[0]
        self.assertEqual(3.00536, timestamp)
        # Elapsed time should equal state.tick_count
        elapsed = struct.unpack('<I', buff[18:22])[0]
        self.assertEqual(state.tick_count, elapsed)
        # Data source is the final byte (23) and should be 0 for now
        self.assertEqual(0, buff[22])

    def test_generate_gnss(self):
        state: State = State()
        # Simulate first time after initial time step
        state.update_ticks(300536)
        state.ref_time.update(state.curr_ticks)
        state.sim_time.update(math.floor(state.curr_ticks / 2))
        # Simulate first position after initial position step
        state.current_longitude = -74.999996729200006
        state.current_latitude = 43.000003270800001
        gen: DataGenerator = DataGenerator()

        bio = io.BytesIO()
        writer = io.BufferedWriter(bio)
        gen.generate_gnss(state, writer)

        writer.flush()
        buff: bytes = bio.getvalue()
        self.assertIsNotNone(buff)
        # Message type is the 1st-4th bytes
        self.assertEqual(PacketTypes.GNSS.value, struct.unpack('<I', buff[0:4])[0])
        # Message length is the 5th-9th bytes
        self.assertEqual(87, struct.unpack('<I', buff[4:8])[0])
        # Days since epoch from bytes 9 and 10
        self.assertEqual(18262, struct.unpack('<H', buff[8:10])[0])
        # Read timestamp from bytes 11-19
        self.assertEqual(3.00536, struct.unpack('<d', buff[10:18])[0])
        # Elapsed time should equal state.tick_count
        self.assertEqual(state.tick_count, struct.unpack('<I', buff[18:22])[0])
        # Message date should equal state.sim_time.days_since_epoch()
        self.assertEqual(state.sim_time.days_since_epoch(), struct.unpack('<H', buff[22:24])[0])
        # Message timestamp = state.sim_time.seconds_in_day()
        self.assertEqual(state.sim_time.seconds_in_day(), struct.unpack('<d', buff[24:32])[0])
        # Latitude
        self.assertEqual(state.current_latitude, struct.unpack('<d', buff[32:40])[0])
        # Longitude
        self.assertEqual(state.current_longitude, struct.unpack('<d', buff[40:48])[0])
        # Hard-coded altitude
        self.assertEqual(-19.323, struct.unpack('<d', buff[48:56])[0])
        # Hard-coded rx_type
        self.assertEqual(0, buff[56])
        # Hard-coded rx_method
        self.assertEqual(2, buff[57])
        # Hard-coded num SVs
        self.assertEqual(12, buff[58])
        # Hard-coded horizontal DOP
        self.assertEqual(1.5, struct.unpack('<d', buff[59:67])[0])
        # Hard-coded position DOP
        self.assertEqual(2.2, struct.unpack('<d', buff[67:75])[0])
        # Hard-coded sep
        self.assertEqual(22.3453, struct.unpack('<d', buff[75:83])[0])
        # Hard-coded n_refs
        self.assertEqual(1, buff[83])
        # Hard-coded refs_type
        self.assertEqual(4, buff[84])
        # Hard-coded refs_id
        self.assertEqual(12312, struct.unpack('<H', buff[85:87])[0])
        # Hard-coded correction_age
        self.assertEqual(2.32, struct.unpack('<d', buff[87:95])[0])

    def test_generate_depth(self):
        state: State = State()
        # Simulate first time after initial time step
        state.update_ticks(300536)
        state.ref_time.update(state.curr_ticks)
        gen: DataGenerator = DataGenerator()

        bio = io.BytesIO()
        writer = io.BufferedWriter(bio)
        gen.generate_depth(state, writer)

        writer.flush()
        buff: bytes = bio.getvalue()
        self.assertIsNotNone(buff)
        # Message type is the 1st-4th bytes
        self.assertEqual(PacketTypes.Depth.value, struct.unpack('<I', buff[0:4])[0])
        # Message length is the 5th-9th bytes
        self.assertEqual(38, struct.unpack('<I', buff[4:8])[0])
        # Days since epoch from bytes 9 and 10
        self.assertEqual(18262, struct.unpack('<H', buff[8:10])[0])
        # Read timestamp from bytes 11-19
        self.assertEqual(3.00536, struct.unpack('<d', buff[10:18])[0])
        # Elapsed time should equal state.tick_count
        self.assertEqual(state.tick_count, struct.unpack('<I', buff[18:22])[0])
        # Depth should equal state.current_depth
        self.assertEqual(state.current_depth, struct.unpack('<d', buff[22:30])[0])
        # Hard-coded offset
        self.assertEqual(0.0, struct.unpack('<d', buff[30:38])[0])
        # Hard-coded range
        self.assertEqual(200.0, struct.unpack('<d', buff[38:46])[0])

    def test_generate_zda(self):
        state: State = State()
        # Simulate first position after initial position step
        state.current_longitude = -74.999996729200006
        state.current_latitude = 43.000003270800001
        # Simulate first time after initial time step
        state.update_ticks(300536)
        state.ref_time.update(state.curr_ticks)
        state.sim_time.update(math.floor(state.curr_ticks / 2))
        gen: DataGenerator = DataGenerator()

        bio = io.BytesIO()
        writer = io.BufferedWriter(bio)
        gen.generate_zda(state, writer)

        writer.flush()
        buff: bytes = bio.getvalue()
        self.assertIsNotNone(buff)

        # Message type is the 1st-4th bytes
        self.assertEqual(PacketTypes.SerialString.value, struct.unpack('<I', buff[0:4])[0])
        # Message length is the 5th-9th bytes
        self.assertEqual(44, struct.unpack('<I', buff[4:8])[0])
        # Days since epoch from bytes 9 and 10
        self.assertEqual(state.tick_count, struct.unpack('<I', buff[8:12])[0])

        # First byte of message body is '$' = ASCII 36 (decimal)
        self.assertEqual(36, buff[12])
        # Message descriptor: GPZDA
        # G = 71
        self.assertEqual(71, buff[13])
        # P = 80
        self.assertEqual(80, buff[14])
        # Z = 90
        self.assertEqual(90, buff[15])
        # D = 68
        self.assertEqual(68, buff[16])
        # A = 65
        self.assertEqual(65, buff[17])
        # Component separator is , = 44
        self.assertEqual(44, buff[18])
        # Hour, minute, and decimal seconds (all initially ASCII 0=48 decimal)
        self.assertEqual(48, buff[19])
        self.assertEqual(48, buff[20])
        self.assertEqual(48, buff[21])
        self.assertEqual(48, buff[22])
        # Seconds '01.503'
        self.assertEqual(48, buff[23])
        # '1' = 49
        self.assertEqual(49, buff[24])
        # '.' = 46
        self.assertEqual(46, buff[25])
        # Decimal part of seconds
        # '5' = 53
        self.assertEqual(53, buff[26])
        self.assertEqual(48, buff[27])
        # '3' = 51
        self.assertEqual(51, buff[28])
        # Component separator is , = 44
        self.assertEqual(44, buff[29])
        # Month day is '31'
        self.assertEqual(b'31', buff[30:32])
        # Component separator is , = 44
        self.assertEqual(44, buff[32])
        # Month is '12'
        self.assertEqual(b'12', buff[33:35])
        # Component separator is , = 44
        self.assertEqual(44, buff[35])
        # Year is '2020'
        self.assertEqual(b'2020', buff[36:40])
        # Component separator is , = 44
        self.assertEqual(44, buff[40])
        # End of message is hard-coded with '00,00*'
        self.assertEqual(b'00,00*', buff[41:47])
        # Component separator is , = 44
        self.assertEqual(44, buff[47])
        # Checksum: '50'
        self.assertEqual(b'50', buff[48:50])
        # End of message: '\r\n'
        self.assertEqual(b'\r\n', buff[50:52])

    def test_generate_dbt(self):
        state: State = State()
        # Simulate first position after initial position step
        state.current_longitude = -74.999996729200006
        state.current_latitude = 43.000003270800001
        # Simulate first time after initial time step
        state.update_ticks(300536)
        state.ref_time.update(state.curr_ticks)
        state.sim_time.update(math.floor(state.curr_ticks / 2))
        gen: DataGenerator = DataGenerator()

        bio = io.BytesIO()
        writer = io.BufferedWriter(bio)
        gen.generate_dbt(state, writer)

        writer.flush()
        buff: bytes = bio.getvalue()
        self.assertIsNotNone(buff)

        # Message type is the 1st-4th bytes
        self.assertEqual(PacketTypes.SerialString.value, struct.unpack('<I', buff[0:4])[0])
        # Message length is the 5th-9th bytes
        # Length may vary a bit due to random depth values, so let's just make sure it's a valid int
        exc_raised = False
        try:
            msg_len = int(struct.unpack('<I', buff[4:8])[0])
        except ValueError:
            exc_raised = True
        self.assertFalse(exc_raised)
        # Days since epoch from bytes 9 and 10
        self.assertEqual(state.tick_count, struct.unpack('<I', buff[8:12])[0])

        # First byte of message body is '$' = ASCII 36 (decimal)
        self.assertEqual(36, buff[12])
        # Message descriptor: SDDBT
        # S = 83
        self.assertEqual(83, buff[13])
        # D = 68
        self.assertEqual(68, buff[14])
        # D = 68
        self.assertEqual(68, buff[15])
        # B = 66
        self.assertEqual(66, buff[16])
        # T = 84
        self.assertEqual(84, buff[17])
        # Component separator is , = 44
        self.assertEqual(44, buff[18])

        # Depth in feet (random value so we just make sure it is a valid float)
        exc_raised = False
        try:
            depth_feet = float(buff[19:23])
        except ValueError:
            exc_raised = True
        self.assertFalse(exc_raised)
        # Component separator is , = 44
        self.assertEqual(44, buff[23])
        # Unit: 'f' = 102
        self.assertEqual(102, buff[24])
        # Component separator is , = 44
        self.assertEqual(44, buff[25])

        # Depth in metres (random value so we just make sure it is a valid float)
        exc_raised = False
        try:
            depth_metres = float(buff[26:30])
        except ValueError:
            exc_raised = True
        self.assertFalse(exc_raised)
        # Component separator is , = 44
        self.assertEqual(44, buff[30])
        # Unit: 'M' = 77
        self.assertEqual(77, buff[31])
        # Component separator is , = 44
        self.assertEqual(44, buff[32])

        # Depth in fathoms (random value so we just make sure it is a valid float)
        exc_raised = False
        try:
            depth_fathoms = float(buff[33:36])
        except ValueError:
            exc_raised = True
        self.assertFalse(exc_raised)
        # Component separator is , = 44
        self.assertEqual(44, buff[36])
        # Unit and message end: 'F*'
        self.assertEqual(b'F*', buff[37:39])
        # Component separator is , = 44
        self.assertEqual(44, buff[39])

        # Checksum may be different each time due to random elements above so just make sure
        # it is an integer in hexadecimal form.
        exc_raised = False
        try:
            depth_fathoms = int(buff[40:42], 16)
        except ValueError:
            exc_raised = True
        self.assertFalse(exc_raised)
        # End of message: '\r\n'
        self.assertEqual(b'\r\n', buff[42:44])


if __name__ == '__main__':
    unittest.main()
