import unittest

from cloud.ai.speechkit.stt.lib.data_pipeline.transcription_tasks import get_bits_overlaps


class TestGetBitsOverlaps(unittest.TestCase):
    def test_get_bits_overlaps(self):
        filenames = [
            'eb67f386-ffa6-4da2-807d-87d93e61e893_1_0-9000.wav',
            'eb67f49f-0b1f-4a29-9d23-b807055dd94a_1_0-1234.wav',
            'eb6813f0-83a3-4408-8c07-9ba90804552f_1_0-9000.wav',
            'eb67f386-ffa6-4da2-807d-87d93e61e893_1_6000-15000.wav',
            'eb6813f0-83a3-4408-8c07-9ba90804552f_1_3000-12000.wav',
            'eb67f386-ffa6-4da2-807d-87d93e61e893_1_3000-12000.wav',
            'eb6813f0-83a3-4408-8c07-9ba90804552f_1_6000-15000.wav',
            'eb6813f0-83a3-4408-8c07-9ba90804552f_1_9000-18000.wav',
        ]
        overlaps = [3, 3, 3, 3, 1, 1, 1, 3]
        answer = dict(zip(filenames, overlaps))
        self.assertEqual(answer, get_bits_overlaps(
            filenames=filenames, overlap=3, offset=3000, basic_overlap=3, edge_bits_full_overlap=True,
        ))
