import unittest

from cloud.ai.speechkit.stt.lib.data_pipeline.transcription_tasks import get_bit_index


class TestGetBitIndex(unittest.TestCase):
    def test_start_is_zero(self):
        self.assertEqual(0, get_bit_index(0, 3000))

    def test_start_is_divisible(self):
        self.assertEqual(5, get_bit_index(15000, 3000))

    def test_start_is_not_divisible_1(self):
        self.assertEqual(4, get_bit_index(9001, 3000))

    def test_start_is_not_divisible_2(self):
        self.assertEqual(4, get_bit_index(9876, 3000))
