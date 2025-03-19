# coding: utf-8
from unittest.mock import MagicMock

from test.mocks import _get_config
from test.tasks.utils import get_task_host

from cloud.mdb.dbaas_worker.internal.tasks.clickhouse.utils import ch_build_host_groups, update_zero_copy_required
from cloud.mdb.internal.python.pytest.utils import parametrize


@parametrize(
    {
        'id': 'Cluster with ZooKeeper, cloud storage enabled, update_zero_copy_schema=True',
        'args': {
            'task_args': {
                'update_zero_copy_schema': True,
            },
            'ch_num': 2,
            'zk_num': 3,
            'cloud_storage_enabled': True,
            'result': True,
        },
    },
    {
        'id': 'Cluster without ZooKeeper, cloud storage enabled, update_zero_copy_schema=True',
        'args': {
            'task_args': {
                'update_zero_copy_schema': True,
            },
            'ch_num': 1,
            'zk_num': 0,
            'cloud_storage_enabled': True,
            'result': False,
        },
    },
    {
        'id': 'Cluster with ZooKeeper, cloud storage disabled, update_zero_copy_schema=True',
        'args': {
            'task_args': {
                'update_zero_copy_schema': True,
            },
            'ch_num': 2,
            'zk_num': 3,
            'cloud_storage_enabled': False,
            'result': False,
        },
    },
    {
        'id': 'Cluster with ZooKeeper, cloud storage enabled, update_zero_copy_schema=False',
        'args': {
            'task_args': {
                'update_zero_copy_schema': False,
            },
            'ch_num': 2,
            'zk_num': 3,
            'cloud_storage_enabled': True,
            'result': False,
        },
    },
    {
        'id': 'Cluster with ZooKeeper, cloud storage enabled, upgrade from 21.8 to 22.3',
        'args': {
            'task_args': {
                'version_from': '21.8.15.7',
                'version_to': '22.3.2.2',
            },
            'ch_num': 2,
            'zk_num': 3,
            'cloud_storage_enabled': True,
            'result': True,
        },
    },
    {
        'id': 'Cluster without ZooKeeper, cloud storage enabled, upgrade from 21.8 to 22.3',
        'args': {
            'task_args': {
                'version_from': '21.8.15.7',
                'version_to': '22.3.2.2',
            },
            'ch_num': 1,
            'zk_num': 0,
            'cloud_storage_enabled': True,
            'result': False,
        },
    },
    {
        'id': 'Cluster with ZooKeeper, cloud storage disabled, upgrade from 21.8 to 22.3',
        'args': {
            'task_args': {
                'version_from': '21.8.15.7',
                'version_to': '22.3.2.2',
            },
            'ch_num': 2,
            'zk_num': 3,
            'cloud_storage_enabled': False,
            'result': False,
        },
    },
    {
        'id': 'Cluster with ZooKeeper, cloud storage enabled, upgrade from 22.1 to 22.3',
        'args': {
            'task_args': {
                'update_zero_copy_schema': False,
                'version_from': '22.1.4.30',
                'version_to': '22.3.2.2',
            },
            'ch_num': 2,
            'zk_num': 3,
            'cloud_storage_enabled': True,
            'result': False,
        },
    },
)
def test_update_zero_copy_required(task_args, zk_num, ch_num, cloud_storage_enabled, result):
    task_args['hosts'] = {}
    for i in range(0, zk_num):
        task_args['hosts'][f'zk_{i}_fqdn'] = get_task_host(vtype='compute', roles='zk')
    for i in range(0, ch_num):
        task_args['hosts'][f'clickhouse_{i}_fqdn'] = get_task_host(vtype='compute', roles='clickhouse')

    config = _get_config()

    ch_group, zk_group = ch_build_host_groups(task_args['hosts'], config)

    cloud_storage = MagicMock()
    cloud_storage.has_exists_cloud_storage_bucket = lambda _: cloud_storage_enabled

    assert update_zero_copy_required(task_args, ch_group, zk_group, cloud_storage) is result
