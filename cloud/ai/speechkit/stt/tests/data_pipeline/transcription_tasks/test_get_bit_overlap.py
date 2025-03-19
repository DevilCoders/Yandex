import unittest

from cloud.ai.speechkit.stt.lib.data_pipeline.transcription_tasks import get_bit_overlap


class TestGetBitOverlap(unittest.TestCase):
    def test_get_bit_overlap_first_bit(self):
        self.assertEqual(3, get_bit_overlap(index=0, size=3, overlap=3, basic_overlap=3, edge_bits_full_overlap=True))

    def test_get_bit_overlap_last_bit(self):
        self.assertEqual(3, get_bit_overlap(index=2, size=3, overlap=3, basic_overlap=3, edge_bits_full_overlap=True))

    def test_get_bit_overlap_middle_bit_overlap_equals3(self):
        self.assertEqual(1, get_bit_overlap(index=1, size=3, overlap=3, basic_overlap=3, edge_bits_full_overlap=True))

    def test_get_bit_overlap_middle_bit_overlap_equals4(self):
        self.assertEqual(1, get_bit_overlap(index=1, size=3, overlap=4, basic_overlap=3, edge_bits_full_overlap=True))

    def test_get_bit_overlap_middle_bit_overlap_equals5_less(self):
        self.assertEqual(2, get_bit_overlap(index=1, size=4, overlap=5, basic_overlap=3, edge_bits_full_overlap=True))

    def test_get_bit_overlap_middle_bit_overlap_equals5_more(self):
        self.assertEqual(1, get_bit_overlap(index=2, size=4, overlap=5, basic_overlap=3, edge_bits_full_overlap=True))

    def test_get_bit_overlap_middle_bit_overlap_equals7_basic_overlap_equals4_less(self):
        self.assertEqual(2, get_bit_overlap(index=10, size=18, overlap=7, basic_overlap=4, edge_bits_full_overlap=True))

    def test_get_bit_overlap_middle_bit_overlap_equals7_basic_overlap_equals4_more(self):
        self.assertEqual(1, get_bit_overlap(index=11, size=18, overlap=7, basic_overlap=4, edge_bits_full_overlap=True))

    def test_get_bit_overlap_edge_bits_full_overlap_disabled(self):
        self.assertEqual(1, get_bit_overlap(index=0, size=3, overlap=3, basic_overlap=3, edge_bits_full_overlap=False))
        self.assertEqual(1, get_bit_overlap(index=2, size=3, overlap=3, basic_overlap=3, edge_bits_full_overlap=False))
