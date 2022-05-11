import unittest
import io

from wibl.simulator.data import DataGenerator, State


class TestDataGenerator(unittest.TestCase):
    def test_compute_checksum(self):
        self.assertEqual(86, DataGenerator.compute_checksum("$GPZDA,000000.000,01,01,2020,00,00*"))

    def test_format_angle(self):
        fa: DataGenerator.FormattedAngle = DataGenerator.format_angle(43.000003270800001)
        self.assertEqual(43, fa.degrees)
        self.assertEqual(0.0000032708000006209659, fa.minutes)
        self.assertEqual(1, fa.hemisphere)

        fa: DataGenerator.FormattedAngle = DataGenerator.format_angle(-74.999996729200006)
        self.assertEqual(74, fa.degrees)
        self.assertEqual(0.99999672920000648, fa.minutes)
        self.assertEqual(0, fa.hemisphere)

    def test_generate_gga(self):
        state: State = State()
        # Simulate first position after initial position step
        state.current_longitude = -74.999996729200006
        state.current_latitude = 43.000003270800001
        gen: DataGenerator = DataGenerator()

        bio = io.BytesIO()
        writer = io.BufferedWriter(bio)
        gen.generate_gga(state, writer)

        writer.flush()
        buff: bytes = bio.getvalue()
        self.assertIsNotNone(buff)
        # Message type is the first byte (little endian)
        self.assertEqual(10, buff[0])
        # Empty high-order bytes
        self.assertEqual(0, buff[1])
        self.assertEqual(0, buff[2])
        self.assertEqual(0, buff[3])
        # Message length is the 5th byte (little endian)
        self.assertEqual(89, buff[4])
        # Empty high-order bytes
        self.assertEqual(0, buff[5])
        self.assertEqual(0, buff[6])
        self.assertEqual(0, buff[7])
        # Tick count is zero initially (4-bytes)
        self.assertEqual(0, buff[8])
        self.assertEqual(0, buff[9])
        self.assertEqual(0, buff[10])
        self.assertEqual(0, buff[11])
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
        self.assertEqual(48, buff[23])
        self.assertEqual(48, buff[24])
        # . = 46
        self.assertEqual(46, buff[25])
        # Decimal part of seconds
        self.assertEqual(48, buff[26])
        self.assertEqual(48, buff[27])
        self.assertEqual(48, buff[28])
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
        self.assertEqual(55, buff[93])
        self.assertEqual(67, buff[94])
        # '\r\n'
        self.assertEqual(13, buff[95])
        self.assertEqual(10, buff[96])


if __name__ == '__main__':
    unittest.main()
