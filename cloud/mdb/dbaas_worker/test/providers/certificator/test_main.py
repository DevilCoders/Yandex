from cloud.mdb.dbaas_worker.internal.providers.certificator import CertificatorApi
from cloud.mdb.dbaas_worker.internal.providers.certificator.base import AvailableCerts
from test.mocks import _get_config
from queue import Queue


def get_provider(config) -> CertificatorApi:
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
    return CertificatorApi(config, task, queue)


def noop_provider() -> CertificatorApi:
    config = _get_config()
    config.cert_api.api = AvailableCerts.noop
    return get_provider(config)


def test_noop_issue_does_not_fail():
    noop_provider().issue('foo.db', [])


def test_noop_revoke_does_not_fail():
    noop_provider().revoke('foo.db')
