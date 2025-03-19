from queue import Queue
from cloud.mdb.dbaas_worker.internal.providers.dns import DnsApi
from test.mocks import _get_config
import pytest


@pytest.mark.parametrize(
    'api',
    ["NOOP", "SLAYER", "YC.DNS"],
)
def test_provider_with_api_can_be_instantiated(api):
    config = _get_config()
    config.dns.api = api
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
    DnsApi(config, task, queue)
