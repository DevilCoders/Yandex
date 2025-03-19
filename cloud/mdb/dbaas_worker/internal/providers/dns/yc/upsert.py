from collections import defaultdict
from typing import NamedTuple
from ..client import Record, normalized_address

RecordsType2Addresses = defaultdict[str, set[str]]


def records_to_type_map(records: list[Record]) -> RecordsType2Addresses:
    expected = RecordsType2Addresses(set)
    for record in records:
        expected[record.record_type].add(normalized_address(record.address, record.record_type))
    return expected


def _rt2a_from_dict(js_d: dict[str, list[str]]) -> RecordsType2Addresses:
    rd = RecordsType2Addresses(set)
    for key, value in js_d.items():
        rd[key] = set(value)
    return rd


def _rt2a_to_dict(rt2a: RecordsType2Addresses) -> dict:
    return {k: list(v) for k, v in rt2a.items()}


class UpsertRequest(NamedTuple):
    merges: RecordsType2Addresses
    deletions: RecordsType2Addresses

    @staticmethod
    def make_empty() -> 'UpsertRequest':
        return UpsertRequest(RecordsType2Addresses(set), RecordsType2Addresses(set))

    def is_empty(self) -> bool:
        return not self.merges and not self.deletions

    def to_serializable(self) -> dict:
        return dict(
            merges=_rt2a_to_dict(self.merges),
            deletions=_rt2a_to_dict(self.deletions),
        )

    @staticmethod
    def from_serializable(plan_dicts: dict) -> 'UpsertRequest':
        return UpsertRequest(
            merges=_rt2a_from_dict(plan_dicts['merges']),
            deletions=_rt2a_from_dict(plan_dicts['deletions']),
        )


class UpsertPlan(NamedTuple):
    plan: UpsertRequest
    rollback_plan: UpsertRequest

    @staticmethod
    def build(actual: RecordsType2Addresses, expected: RecordsType2Addresses) -> 'UpsertPlan':
        plan = UpsertRequest.make_empty()
        rollback_plan = UpsertRequest.make_empty()

        # There are 3 cases:
        # 1. New record
        #   - Add it to merges
        #   - Add it to rollback_deletions
        # 2. We modify record
        #   - Add it to merges
        #   - Add old to merges
        # 3. We delete record
        #   - Add it to deletions
        #   - Add old to rollback_merges

        for record_type, expected_addresses in expected.items():
            # First case - new record
            if record_type not in actual:
                plan.merges[record_type] |= expected_addresses
                rollback_plan.deletions[record_type] |= expected_addresses
                continue

            if actual[record_type] == expected_addresses:
                continue

            # Second case - changed record
            plan.merges[record_type] |= expected_addresses
            rollback_plan.merges[record_type] |= actual[record_type]

        # Handle third case - we delete record
        for record_type in set(actual) - set(expected):
            plan.deletions[record_type] |= actual[record_type]
            rollback_plan.merges[record_type] |= actual[record_type]
        return UpsertPlan(
            plan=plan,
            rollback_plan=rollback_plan,
        )
