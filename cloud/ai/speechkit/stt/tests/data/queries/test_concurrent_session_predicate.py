import unittest

from cloud.ai.speechkit.stt.lib.data.ops.queries import get_uuid_range_predicate


class TestRangeQuery(unittest.TestCase):
    def test_get_uuid_range_predicate(self):
        for inputs, results in [
            ((1, 2), ('0', '7')),
            ((2, 2), ('8', 'f')),
            ((1, 4), ('0', '3')),
            ((3, 4), ('8', 'b')),
            ((7, 8), ('c', 'd')),
            ((5, 16), ('4', '4'))
        ]:
            self.assertEqual(f'SUBSTRING(records.`id`, 0, 1) BETWEEN "{results[0]}" AND "{results[1]}"',
                             get_uuid_range_predicate(*inputs)
                             )

    def test_get_uuid_incorrect(self):
        with self.assertRaises(Exception):
            get_uuid_range_predicate(-1, 8)

        with self.assertRaises(Exception):
            get_uuid_range_predicate(0, 8)

        with self.assertRaises(Exception):
            get_uuid_range_predicate(1, 1)

        with self.assertRaises(Exception):
            get_uuid_range_predicate(1, 12)

        with self.assertRaises(Exception):
            get_uuid_range_predicate(10, 2)
