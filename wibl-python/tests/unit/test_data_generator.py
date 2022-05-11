import unittest

from wibl.simulator.data import DataGenerator


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


if __name__ == '__main__':
    unittest.main()
