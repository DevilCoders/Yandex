import unittest

from cloud.ai.speechkit.stt.lib.data_pipeline.records_splitting import get_bits_indices


class TestGetBitsIndices(unittest.TestCase):
    def test__bit_length_more_than_length(self):
        self.assertEqual([(0, 5432)], get_bits_indices(length=5432, bit_length=9000, bit_offset=3000))

    def test__bit_length_equal_to_length(self):
        self.assertEqual([(0, 9000)], get_bits_indices(length=9000, bit_length=9000, bit_offset=3000))

    def test__bit_length_less_than_length__bit_offset_divides_length__bit_offset_divides_bit_length_1(self):
        self.assertEqual(
            [(0, 9000), (3000, 12000), (6000, 15000)], get_bits_indices(length=15000, bit_length=9000, bit_offset=3000)
        )

    def test__bit_length_less_than_length__bit_offset_divides_length__bit_offset_divides_bit_length_2(self):
        self.assertEqual(
            [(0, 9000), (3000, 12000), (6000, 15000), (9000, 18000), (12000, 21000)],
            get_bits_indices(length=21000, bit_length=9000, bit_offset=3000),
        )

    def test__bit_length_less_than_length__bit_offset_not_divides_length__bit_offset_divides_bit_length_1(self):
        self.assertEqual([(0, 9000), (3000, 9001)], get_bits_indices(length=9001, bit_length=9000, bit_offset=3000))

    def test__bit_length_less_than_length__bit_offset_not_divides_length__bit_offset_divides_bit_length_2(self):
        self.assertEqual(
            [(0, 9000), (3000, 12000), (6000, 15000), (9000, 18000), (12000, 21000), (15000, 23428)],
            get_bits_indices(length=23428, bit_length=9000, bit_offset=3000),
        )

    def test__bit_length_less_than_length__bit_offset_not_divides_length__bit_offset_not_divides_bit_length_1(self):
        self.assertEqual(
            [(0, 5000), (3000, 8000), (6000, 9001)], get_bits_indices(length=9001, bit_length=5000, bit_offset=3000)
        )

    def test__bit_length_less_than_length__bit_offset_not_divides_length__bit_offset_not_divides_bit_length_2(self):
        self.assertEqual(
            [(0, 5000), (3000, 8000), (6000, 11000), (9000, 14000), (12000, 14228)],
            get_bits_indices(length=14228, bit_length=5000, bit_offset=3000),
        )

    def test__bit_length_less_than_length__bit_offset_divides_length__bit_offset_not_divides_bit_length_1(self):
        self.assertEqual(
            [(0, 5000), (3000, 8000), (6000, 9000)], get_bits_indices(length=9000, bit_length=5000, bit_offset=3000)
        )

    def test__bit_length_less_than_length__bit_offset_divides_length__bit_offset_not_divides_bit_length_2(self):
        self.assertEqual(
            [(0, 5000), (3000, 8000), (6000, 11000), (9000, 14000), (12000, 15000)],
            get_bits_indices(length=15000, bit_length=5000, bit_offset=3000),
        )
