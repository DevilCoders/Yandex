import unittest

from cloud.ai.speechkit.stt.lib.data_pipeline.markup_cost import (
    calculate_bits_count,
    calculate_assignments_count,
    calculate_final_billing_units,
    calculate_billing_unit_rate,
)


class TestGetBitsIndices(unittest.TestCase):
    def test_calculate_bits_count(self):
        for expected_bits_count, records_durations, records_channel_counts in [
            (3, [1000], [1]),
            (6, [1000], [2]),
            (3, [9000], [1]),
            (6, [10000], [1]),
            (10, [22000], [1]),
            (22, [1000, 9000, 10000, 22000], [1, 1, 1, 1]),
            (37, [1000, 9000, 10000, 22000], [2, 1, 3, 1]),
        ]:
            self.assertEqual(
                expected_bits_count,
                calculate_bits_count(
                    records_durations=records_durations,
                    records_channel_counts=records_channel_counts,
                    bit_length=9000,
                    bit_offset=3000,
                    chunk_overlap=3,
                    edge_bits_full_overlap=True,
                ),
            )

    def test_calculate_assignments_count(self):
        for expected_assignments_count, bits_count in [
            (1, 1),
            (1, 20),
            (2, 21),
            (6, 101),
        ]:
            self.assertEqual(
                expected_assignments_count,
                calculate_assignments_count(
                    bits_count=bits_count,
                    real_tasks_per_assignment=20,
                ),
            )

    def test_calculate_final_billing_units(self):
        for expected_cost, records_durations, records_channel_counts in [
            (60, [1], [1]),
            (60, [1], [2]),
            (60, [9] * 400, [1] * 400),
            (61, [9] * 401, [1] * 401),
            (91, [9] * 401, [1] * 200 + [2] * 201),
        ]:
            self.assertEqual(
                expected_cost,
                calculate_final_billing_units(
                    records_durations=records_durations,
                    records_channel_counts=records_channel_counts,
                    bit_length=9,
                    bit_offset=3,
                    chunk_overlap=3,
                    edge_bits_full_overlap=True,
                    real_tasks_per_assignment=20,
                ),
            )

    def test_calculate_billing_unit_rate(self):
        self.assertEqual(
            36.0, calculate_billing_unit_rate(reward_per_assignment_usd=0.2, rub_in_usd=75.0, cost_multiplier=2.0)
        )
