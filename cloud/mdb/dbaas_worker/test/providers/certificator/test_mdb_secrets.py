from mock import Mock
from cloud.mdb.dbaas_worker.internal.providers.certificator.mdb_secrets import MDBSecretsApi
from test.mocks import _get_config
from queue import Queue


def get_mdb_secrets() -> MDBSecretsApi:
    config = _get_config()
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
    return MDBSecretsApi(config, task, queue)


def test_secrets_revokes(mocker):
    session = mocker.patch('cloud.mdb.dbaas_worker.internal.providers.http.requests.Session').return_value
    session.request.side_effect = [
        Mock(
            status_code=200,
            json=lambda *args: {},
            headers={
                'Content-Type': 'application/json',
            },
        )
    ]
    secrets = get_mdb_secrets()
    secrets.revoke('test-host')
    session.request.assert_called_once()


def test_secrets_revokes_when_cert_is_deleted(mocker):
    session = mocker.patch('cloud.mdb.dbaas_worker.internal.providers.http.requests.Session').return_value
    session.request.side_effect = [
        Mock(
            status_code=404,
            json=lambda *args: {},
            headers={
                'Content-Type': 'application/json',
            },
        )
    ]
    secrets = get_mdb_secrets()
    secrets.revoke('test-host')
    session.request.assert_called_once()
