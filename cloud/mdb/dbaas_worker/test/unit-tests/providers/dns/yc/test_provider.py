from queue import Queue
import mock
import pytest
from cloud.mdb.dbaas_worker.internal.providers.dns import Record
from cloud.mdb.dbaas_worker.internal.providers.dns.yc import YCDns
from cloud.mdb.dbaas_worker.internal.exceptions import Interrupted
from cloud.mdb.internal.python.compute.dns import DnsRecordSet
from cloud.mdb.internal.python.compute.models import OperationModel
from test.mocks import _get_config


def new_provider() -> YCDns:
    queue = Queue(maxsize=10000)
    task = {
        'cid': 'cid-test',
        'task_id': 'test_id',
        'task_type': 'test-task',
        'feature_flags': [],
        'folder_id': 'test_folder',
        'context': {},
        'timeout': 3600,
        'changes': [],
    }
    config = _get_config()
    config.dns.api = 'YC.DNS'
    config.yc_dns.zones = {'db.test.': 'yc.mdb.test'}
    config.yc_dns.ignored_zones = frozenset(['ignored.zone.'])
    config.yc_dns.operation_wait_timeout = 10
    config.yc_dns.operation_wait_step = 0.0
    return YCDns(config, task, queue)


def new_op(operation_id: str, done: bool) -> OperationModel:
    op = OperationModel()
    op.operation_id = operation_id
    op.done = done
    op.error = None
    return op


class TestYCDns:
    def test_set_records_for_unsupported_zone(self):
        with pytest.raises(RuntimeError):
            new_provider().set_records('name-in.unsupported.zone', [])

    def test_set_records_for_ignored_zone(self):
        new_provider().set_records('name-in.ignored.zone', [])

    def test_set_records_ignore_aaaa(self):
        provider = new_provider()
        with mock.patch.object(provider, '_dns_client') as dns_client_mock:
            dns_client_mock.list_records.return_value = []
            dns_client_mock.upsert.return_value = new_op('op_id', False)
            dns_client_mock.get_operation.return_value = new_op('op_id', True)

            provider.set_records('name.db.test', [Record('10.10.10.10', 'A'), Record('dead:beef', 'AAAA')])
            dns_client_mock.upsert.assert_called_once_with(
                dns_zone_id='yc.mdb.test',
                deletions=[],
                merges=[
                    DnsRecordSet(
                        name='name.db.test.',
                        type='A',
                        ttl=300,
                        data=['10.10.10.10'],
                    )
                ],
            )
            dns_client_mock.get_operation.assert_called()

    def test_set_records_waits_until_operation_complete(self):
        provider = new_provider()
        with mock.patch.object(provider, '_dns_client') as dns_client_mock:
            dns_client_mock.list_records.return_value = []
            dns_client_mock.upsert.return_value = new_op('op_id', False)
            dns_client_mock.get_operation.side_effect = [new_op('op_id', False), new_op('op_id', True)]

            provider.set_records('name.db.test', [Record('10.10.10.10', 'A'), Record('dead:beef', 'AAAA')])
            assert dns_client_mock.get_operation.call_count == 2

    def test_set_records_when_they_already_set(self):
        provider = new_provider()
        with mock.patch.object(provider, '_dns_client') as dns_client_mock:
            dns_client_mock.list_records.return_value = [
                DnsRecordSet(
                    name='name.db.test.',
                    type='A',
                    ttl=300,
                    data=['10.10.10.10'],
                )
            ]

            provider.set_records('name.db.test', [Record('10.10.10.10', 'A')])
            dns_client_mock.upsert.assert_not_called()

    def test_set_records_rollback(self):
        provider = new_provider()
        with mock.patch.object(provider, '_dns_client') as dns_client_mock:
            dns_client_mock.list_records.return_value = []
            dns_client_mock.get_operation.return_value = new_op('op_id', True)

            provider.set_records('name.db.test', [Record('10.10.10.10', 'A'), Record('dead:beef', 'AAAA')])

            dns_client_mock.reset_mock()
            for change in provider.task['changes']:
                change.rollback(provider.task, 42)
            dns_client_mock.upsert.assert_called_once_with(
                dns_zone_id='yc.mdb.test',
                deletions=[
                    DnsRecordSet(
                        name='name.db.test.',
                        type='A',
                        ttl=300,
                        data=['10.10.10.10'],
                    )
                ],
                merges=[],
            )

    def test_set_records_restored_from_context(self):
        initial_provider = new_provider()
        initial_provider.task['context']['interrupted'] = True
        with mock.patch.object(initial_provider, '_dns_client') as dns_client_mock:
            dns_client_mock.list_records.return_value = []
            dns_client_mock.upsert.return_value = new_op('op_id_initiated_before_interrupt', False)
            dns_client_mock.get_operation.side_effect = RuntimeError(
                'Provider should exit before operation status check'
            )

            with pytest.raises(Interrupted):
                initial_provider.set_records('name.db.test', [Record('10.10.10.10', 'A'), Record('dead:beef', 'AAAA')])

            dns_client_mock.get_operation.assert_not_called()
            dns_client_mock.upsert.assert_called()

        restored_provider = new_provider()
        task_context = initial_provider.task['context']
        task_context.pop('interrupted')
        restored_provider.task['context'] = task_context
        with mock.patch.object(restored_provider, '_dns_client') as dns_client_mock:
            dns_client_mock.get_operation.return_value = new_op('op_id_initiated_before_interrupt', True)

            restored_provider.set_records('name.db.test', [Record('10.10.10.10', 'A'), Record('dead:beef', 'AAAA')])
            dns_client_mock.list_records.assert_not_called()
            dns_client_mock.upsert.assert_not_called()
            dns_client_mock.get_operation.assert_called_once_with('op_id_initiated_before_interrupt')

            # rollback should works too

            dns_client_mock.reset_mock()
            for change in restored_provider.task['changes']:
                change.rollback(restored_provider.task, 42)
            dns_client_mock.upsert.assert_called_once_with(
                dns_zone_id='yc.mdb.test',
                deletions=[
                    DnsRecordSet(
                        name='name.db.test.',
                        type='A',
                        ttl=300,
                        data=['10.10.10.10'],
                    )
                ],
                merges=[],
            )
