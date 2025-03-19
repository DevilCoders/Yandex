# coding: utf-8
from queue import Queue
from cloud.mdb.dbaas_worker.internal.tasks.common.cluster.delete_metadata import ClusterDeleteMetadataExecutor
from test.mocks import _get_config
from test.tasks.utils import get_task_host
from unittest.mock import MagicMock


def create_executor(mocker, args):
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
    config.per_cluster_service_accounts.folder_id = 'folder_id'

    mocker.patch('cloud.mdb.dbaas_worker.internal.tasks.common.cluster.delete_metadata.ConductorApi')
    mocker.patch('cloud.mdb.dbaas_worker.internal.tasks.common.cluster.delete_metadata.SolomonApiV2')
    mocker.patch('cloud.mdb.dbaas_worker.internal.tasks.common.cluster.delete_metadata.BaseDeleteMetadataExecutor')
    mocker.patch('cloud.mdb.dbaas_worker.internal.tasks.common.delete_metadata.DnsApi')
    mocker.patch('cloud.mdb.dbaas_worker.internal.tasks.common.delete_metadata.EdsApi')
    mocker.patch('cloud.mdb.dbaas_worker.internal.tasks.common.delete_metadata.Route53')
    mocker.patch('cloud.mdb.dbaas_worker.internal.tasks.common.delete_metadata.ConductorApi')
    mocker.patch('cloud.mdb.dbaas_worker.internal.tasks.common.delete_metadata.JugglerApi')
    mocker.patch('cloud.mdb.dbaas_worker.internal.tasks.common.delete_metadata.CertificatorApi')
    mocker.patch('cloud.mdb.dbaas_worker.internal.tasks.common.delete_metadata.InternalApi')

    return ClusterDeleteMetadataExecutor(config, task, queue, args)


def mock_iam(mocker):
    mock_iam_class = mocker.patch('cloud.mdb.dbaas_worker.internal.tasks.common.cluster.delete_metadata.Iam')
    iam_instance = mock_iam_class.return_value

    return iam_instance


def scenario_service_account_is_removed(mocker, vtype):
    iam = mock_iam(mocker)
    service_account_mock = MagicMock()
    iam.find_service_account_by_name.return_value = service_account_mock

    executor = create_executor(mocker, {'hosts': {'host1': get_task_host(vtype=vtype, subcid='subcid-value')}})
    executor._delete_service_account()

    iam.reconnect.assert_called_once_with()
    iam.find_service_account_by_name.assert_called_once_with('folder_id', 'cluster-agent-cid-test')
    iam.delete_service_account.assert_called_once_with(service_account_mock.id)


def test_delete_service_account_when_host_is_aws_should_remove_service_account(mocker):
    scenario_service_account_is_removed(mocker, 'aws')


def test_delete_service_account_when_host_is_compute_should_remove_service_account(mocker):
    scenario_service_account_is_removed(mocker, 'compute')


def test_delete_service_account_when_host_is_porto_should_not_do_anything(mocker):
    iam = mock_iam(mocker)

    executor = create_executor(mocker, {'hosts': {'host1': get_task_host(vtype='porto', subcid='subcid-value')}})
    executor._delete_service_account()

    iam.reconnect.assert_not_called()
    iam.find_service_account_by_name.assert_not_called()
    iam.delete_service_account.assert_not_called()
