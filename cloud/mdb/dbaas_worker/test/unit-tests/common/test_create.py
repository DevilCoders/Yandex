# coding: utf-8
from queue import Queue
from cloud.mdb.dbaas_worker.internal.tasks.common.create import BaseCreateExecutor
from test.mocks import _get_config, get_state, setup_mocks
from test.tasks.utils import get_task_host


def create_executor():
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

    return BaseCreateExecutor(config, task, queue, {})


def mock_iam(mocker, create_key_value, create_service_account_value):
    mock_iam_class = mocker.patch('cloud.mdb.dbaas_worker.internal.tasks.common.create.Iam')
    iam_instance = mock_iam_class.return_value
    iam_instance.create_key.return_value = create_key_value
    iam_instance.create_service_account.return_value = create_service_account_value

    return iam_instance


def mock_encrypt(mocker, encrypted_data):
    encrypt = mocker.patch('cloud.mdb.dbaas_worker.internal.tasks.common.create.encrypt')
    encrypt.return_value = encrypted_data

    return encrypt


def mock_pillar(mocker):
    return mocker.patch('cloud.mdb.dbaas_worker.internal.tasks.common.create.DbaasPillar').return_value


def check_create_service_account_when_vtype_is_allowed_should_create_service_account_and_grant_roles(mocker, vtype):
    ENCRYPTED_DATA = {'encryption_version': 1, 'data': 'encrypted_data'}
    CLUSTER_RESOURCE_NAME = 'cluster_resource_name'
    SERVICE_ACCOUNT_ID = 'service_account_id_value'
    SERVICE_ACCOUNT_KEY = 'service_account_key_id_value'
    SERVICE_ACCOUNT_PRIVATE_KEY = 'service_account_private_key_value'
    HOST = get_task_host(vtype=vtype, subcid='subcid-value')

    state = get_state()

    iam = mock_iam(mocker, (SERVICE_ACCOUNT_KEY, SERVICE_ACCOUNT_PRIVATE_KEY), SERVICE_ACCOUNT_ID)
    encrypt = mock_encrypt(mocker, ENCRYPTED_DATA)
    setup_mocks(mocker, state)
    pillar = mock_pillar(mocker)
    executor = create_executor()

    executor._create_service_account(CLUSTER_RESOURCE_NAME, {'host1': HOST})

    iam.reconnect.assert_called_once_with()
    iam.create_service_account.assert_called_once_with('folder_id', 'cluster-agent-cid-test')
    iam.grant_role.assert_called_once_with(
        service_account_id=SERVICE_ACCOUNT_ID,
        role='internal.mdb.clusterAgent',
        cluster_resource_type=CLUSTER_RESOURCE_NAME,
        cluster_id=executor.task['cid'],
    )
    iam.create_key.assert_called_once_with(SERVICE_ACCOUNT_ID)
    encrypt.assert_called_once_with(executor.config, SERVICE_ACCOUNT_PRIVATE_KEY)
    pillar.exists.assert_called_once_with(
        'subcid',
        HOST['subcid'],
        ['data', 'service_account'],
        ['id', 'key_id', 'private_key'],
        [SERVICE_ACCOUNT_ID, SERVICE_ACCOUNT_KEY, ENCRYPTED_DATA],
    )


def test_create_service_account_when_vtype_is_compute_should_create_service_account_and_grant_roles(mocker):
    check_create_service_account_when_vtype_is_allowed_should_create_service_account_and_grant_roles(mocker, 'compute')


def test_create_service_account_when_vtype_is_aws_should_create_service_account_and_grant_roles(mocker):
    check_create_service_account_when_vtype_is_allowed_should_create_service_account_and_grant_roles(mocker, 'aws')


def test_create_service_account_when_vtype_is_not_aws_or_compute_should_do_nothing(mocker):
    HOST = get_task_host(vtype='porto', subcid='subcid-value')
    CLUSTER_RESOURCE_NAME = 'cluster_resource_name'
    iam = mock_iam(mocker, None, None)

    state = get_state()
    setup_mocks(mocker, state)

    executor = create_executor()

    executor._create_service_account(CLUSTER_RESOURCE_NAME, {'host1': HOST})

    iam.create_service_account.assert_not_called()
    iam.grant_role.assert_not_called()
