import json
import pytest
from cloud.mdb.dbaas_worker.internal.providers.dns.yc.upsert import UpsertPlan, UpsertRequest


class TestUpsertPlan:
    @pytest.mark.parametrize(
        'records',
        [
            {},
            {'A': {'10.10.10.10'}},
            {'AAAA': {'dead:beef'}},
            {'AAAA': {'dead:beef'}, 'A': {'10.10.10.10'}},
        ],
    )
    def test_set_same_records_produce_empty_plan(self, records):
        assert UpsertPlan.build(records, records) == UpsertPlan(UpsertRequest.make_empty(), UpsertRequest.make_empty())

    def test_delete_all_record(self):
        upsert = UpsertPlan.build({'AAAA': {'dead:beef'}, 'A': {'10.10.10.10'}}, {})
        assert upsert == UpsertPlan(
            plan=UpsertRequest(
                deletions={'AAAA': {'dead:beef'}, 'A': {'10.10.10.10'}},
                merges={},
            ),
            rollback_plan=UpsertRequest(
                deletions={},
                merges={'AAAA': {'dead:beef'}, 'A': {'10.10.10.10'}},
            ),
        )

    def test_add_new_records(self):
        upsert = UpsertPlan.build(
            {},
            {'AAAA': {'dead:beef'}, 'A': {'10.10.10.10'}},
        )
        assert upsert == UpsertPlan(
            plan=UpsertRequest(
                deletions={},
                merges={'AAAA': {'dead:beef'}, 'A': {'10.10.10.10'}},
            ),
            rollback_plan=UpsertRequest(
                deletions={'AAAA': {'dead:beef'}, 'A': {'10.10.10.10'}},
                merges={},
            ),
        )

    def test_add_one_new_records(self):
        upsert = UpsertPlan.build(
            {'AAAA': {'dead:beef'}},
            {'AAAA': {'dead:beef'}, 'A': {'10.10.10.10'}},
        )
        assert upsert == UpsertPlan(
            plan=UpsertRequest(
                deletions={},
                merges={'A': {'10.10.10.10'}},
            ),
            rollback_plan=UpsertRequest(
                deletions={'A': {'10.10.10.10'}},
                merges={},
            ),
        )

    def test_modify_one_record(self):
        upsert = UpsertPlan.build(
            {'AAAA': {'dead:beef'}, 'A': {'10.10.10.10'}},
            {'AAAA': {'dead:beef'}, 'A': {'11.11.11.11'}},
        )
        assert upsert == UpsertPlan(
            plan=UpsertRequest(
                deletions={},
                merges={'A': {'11.11.11.11'}},
            ),
            rollback_plan=UpsertRequest(
                deletions={},
                merges={'A': {'10.10.10.10'}},
            ),
        )

    def test_modify_all_records(self):
        upsert = UpsertPlan.build(
            {'AAAA': {'fe80::1'}, 'A': {'10.10.10.10'}},
            {'AAAA': {'dead:beef'}, 'A': {'11.11.11.11'}},
        )
        assert upsert == UpsertPlan(
            plan=UpsertRequest(
                deletions={},
                merges={'AAAA': {'dead:beef'}, 'A': {'11.11.11.11'}},
            ),
            rollback_plan=UpsertRequest(
                deletions={},
                merges={'AAAA': {'fe80::1'}, 'A': {'10.10.10.10'}},
            ),
        )


class TestUpsertRequest:
    def test_serializable(self):
        req = UpsertRequest(
            deletions={'A': {'10.10.10.10'}},
            merges={'AAAA': {'dead:beef'}, 'A': {'11.11.11.11'}},
        )

        assert req == UpsertRequest.from_serializable(json.loads(json.dumps(req.to_serializable())))
