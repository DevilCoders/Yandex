from queue import Queue

from mock import Mock

from cloud.mdb.dbaas_worker.internal.providers.resource_manager import ResourceManagerApi
from test.mocks import _get_config


def test_check_folder_role_happy_path(mocker):
    """
    True result is on the second page
    """
    expected_service_account_id = '1'
    expected_role_id = '2'
    jwt = mocker.patch('cloud.mdb.dbaas_worker.internal.providers.resource_manager.IamJwt').return_value
    jwt.get_token.side_effect = 'some-jwt-token'
    session = mocker.patch('cloud.mdb.dbaas_worker.internal.providers.http.requests.Session').return_value
    session.request.side_effect = [
        _response(
            bindings=[
                _binding(
                    sa_id='some-other',
                    role_id='some-other',
                )
            ],
            token='next-token',
        ),
        _response(
            bindings=[
                _binding(
                    sa_id=expected_service_account_id,
                    role_id=expected_role_id,
                )
            ],
            token='',
        ),
    ]
    assert get_resource_manager()._get_folder_roles(
        folder_id='some-folder-id',
        service_account_id=expected_service_account_id,
    ) == {expected_role_id}
    assert session.request.call_count == 2
    assert jwt.get_token.call_count == 2


def get_resource_manager() -> ResourceManagerApi:
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
    return ResourceManagerApi(config, task, queue)


def _binding(sa_id: str, role_id: str) -> dict:
    return {
        'subject': {
            'type': 'serviceAccount',
            'id': sa_id,
        },
        'roleId': role_id,
    }


def _list_of_bingings(bindings: list, token: str):
    return {
        'accessBindings': bindings,
        'nextPageToken': token,
    }


def _response(bindings: list, token: str) -> Mock:
    return Mock(
        status_code=200,
        json=lambda *args: _list_of_bingings(
            bindings=bindings,
            token=token,
        ),
        headers={
            'Content-Type': 'application/json',
        },
    )
