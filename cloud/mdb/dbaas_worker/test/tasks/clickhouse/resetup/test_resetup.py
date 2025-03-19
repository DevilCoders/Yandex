from cloud.mdb.dbaas_worker.internal.environment import EnvironmentName
from test.tasks.clickhouse.utils import (
    get_clickhouse_compute_host,
    get_clickhouse_porto_host,
    get_zookeeper_compute_host,
)
from test.tasks.utils import (
    checked_run_task_with_mocks,
    check_task_interrupt_consistency,
    check_mlock_usage,
    check_mlock_skipped,
    check_dbm_transfer_called,
    check_dbm_without_transfer,
    check_compute_delete_instance_called,
    check_compute_delete_instance_not_called,
    prepare_state,
)


def set_env(mocker, env: EnvironmentName):
    get_env_name = mocker.patch('cloud.mdb.dbaas_worker.internal.tasks.common.resetup_common.get_env_name_from_config')
    get_env_name.return_value = env


def env_porto(mocker):
    set_env(mocker, EnvironmentName.PORTO)


def env_compute(mocker):
    set_env(mocker, EnvironmentName.COMPUTE)


def test_porto_clickhouse_cluster_readd_online_interrupt_consistency(mocker):
    """
    Check porto readd interruptions
    """
    env_porto(mocker)
    target_host, target_fqdn = get_clickhouse_porto_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_clickhouse_porto_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_clickhouse_porto_host(geo='geo3'), 'baz.mdb..net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            fqdn2: host2,
            fqdn3: host3,
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )
    target_host['fqdn'] = target_fqdn
    host2['fqdn'] = fqdn2
    host3['fqdn'] = fqdn3
    state['metadb']['queries'] = [
        {
            'query': 'generic_resolve',
            'kwargs': {'cid': 'cid-test'},
            'result': [target_host, host2, host3],
        },
        {
            'query': 'get_pillar',
            'kwargs': {'cid': 'cid-test'},
            'result': [{'value': {}}],
        },
        {
            'query': 'get_pillar',
            'kwargs': {'path': '{data,service_account_id}'},
            'result': [{'value': None}],
        },
    ] + state['metadb']['queries']
    check_task_interrupt_consistency(
        mocker,
        'clickhouse_cluster_online_resetup',
        {
            'fqdn': target_fqdn,
            'resetup_action': 'readd',
            'preserve_if_possible': True,
            'ignore_hosts': [],
            'lock_is_already_taken': True,
            'try_save_disks': True,
            'cid': 'cid-test',
            'resetup_from': 'dom0.mdb.yacloud.net',
        },
        state,
        ignore=[
            'foo.mdb.yacloud.net:unregister initiated',
        ],
    )


def test_porto_clickhouse_cluster_restore_online_interrupt_consistency(mocker):
    """
    Check porto restore interruptions
    """
    env_porto(mocker)
    target_host, target_fqdn = get_clickhouse_porto_host(geo='geo1'), 'foo.mdb.yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            'host3': get_zookeeper_compute_host(geo='geo1'),
            'host4': get_zookeeper_compute_host(geo='geo2'),
            'host5': get_zookeeper_compute_host(geo='geo3'),
        },
        's3_bucket': 'test-s3-bucket',
    }

    *_, state = checked_run_task_with_mocks(
        mocker,
        'clickhouse_cluster_create',
        args,
    )

    target_host['fqdn'] = target_fqdn
    state['metadb']['queries'] = [
        {
            'query': 'generic_resolve',
            'kwargs': {'cid': 'cid-test'},
            'result': [target_host],
        },
        {
            'query': 'get_pillar',
            'kwargs': {},
            'result': [{'value': {}}],
        },
        {
            'query': 'add_cluster_target_pillar',
            'kwargs': {},
            'result': [],
        },
    ] + state['metadb']['queries']
    state['internal-api-config']['backups'] = {
        'cid-test': {
            'backups': [
                {'id': 'cid-test:shard:', 'sourceShardNames': ['shard1']},
            ],
        },
    }
    check_task_interrupt_consistency(
        mocker,
        'clickhouse_cluster_online_resetup',
        {
            'fqdn': target_fqdn,
            'resetup_action': 'restore',
            'preserve_if_possible': True,
            'ignore_hosts': [],
            'lock_is_already_taken': True,
            'try_save_disks': True,
            'cid': 'cid-test',
            'resetup_from': 'dom0.mdb.yacloud.net',
        },
        state,
        ignore=[
            'foo.mdb.yacloud.net:unregister initiated',
        ],
    )


def test_compute_clickhouse_cluster_restore_online_interrupt_consistency(mocker):
    """
    Check compute restore interruptions
    """
    env_compute(mocker)
    target_host, target_fqdn = get_clickhouse_compute_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_clickhouse_compute_host(geo='geo2'), 'bar.mdb.yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            fqdn2: host2,
            'host3': get_zookeeper_compute_host(geo='geo1'),
            'host4': get_zookeeper_compute_host(geo='geo2'),
            'host5': get_zookeeper_compute_host(geo='geo3'),
        },
        's3_bucket': 'test-s3-bucket',
    }

    *_, state = checked_run_task_with_mocks(
        mocker,
        'clickhouse_cluster_create',
        args,
    )
    target_host['fqdn'] = target_fqdn
    target_host['shard_id'] = 'shard1'
    target_host['shard_name'] = 'shard1'
    host2['fqdn'] = fqdn2
    host2['shard_id'] = 'shard2'
    state['metadb']['queries'] = [
        {
            'query': 'generic_resolve',
            'kwargs': {'cid': 'cid-test'},
            'result': [target_host, host2],
        },
        {
            'query': 'get_pillar',
            'kwargs': {},
            'result': [{'value': {}}],
        },
        {
            'query': 'add_cluster_target_pillar',
            'kwargs': {},
            'result': [],
        },
        {
            'query': 'get_pillar',
            'kwargs': {'cid': 'cid-test'},
            'result': [{'value': {}}],
        },
    ] + state['metadb']['queries']
    state['internal-api-config']['backups'] = {
        'cid-test': {
            'backups': [
                {'id': 'cid-test:shard:', 'sourceShardNames': ['shard1']},
            ],
        },
    }
    state['conductor']['groups']['db_cid_test'] = {
        'id': 100,
        'name': 'db_cid_test',
        'parent_ids': [],
        'project': {
            'id': 1,
        },
    }
    check_task_interrupt_consistency(
        mocker,
        'clickhouse_cluster_online_resetup',
        {
            'fqdn': target_fqdn,
            'resetup_action': 'restore',
            'preserve_if_possible': True,
            'ignore_hosts': [],
            'lock_is_already_taken': True,
            'try_save_disks': False,
            'cid': 'cid-test',
            'resetup_from': 'someInstanceID',
        },
        state,
        ignore=[
            'foo.mdb.yacloud.net:unregister initiated',
            'dns.foo.db.yandex.net-AAAA-2001:db8:1::4:removed',
        ],
    )


def test_compute_clickhouse_cluster_readd_online_interrupt_consistency(mocker):
    """
    Check compute readd interruptions
    """
    env_compute(mocker)
    target_host, target_fqdn = get_clickhouse_compute_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_clickhouse_compute_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_clickhouse_compute_host(geo='geo3'), 'baz.mdb.yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            fqdn2: host2,
            fqdn3: host3,
        },
        's3_bucket': 'test-s3-bucket',
        'zk_hosts': 'test-zk',
    }

    *_, state = checked_run_task_with_mocks(
        mocker,
        'clickhouse_cluster_create',
        args,
    )

    target_host['fqdn'] = target_fqdn
    host2['fqdn'] = fqdn2
    host3['fqdn'] = fqdn3
    state['metadb']['queries'] = [
        {
            'query': 'generic_resolve',
            'kwargs': {'cid': 'cid-test'},
            'result': [target_host, host2, host3],
        },
        {
            'query': 'get_pillar',
            'kwargs': {'cid': 'cid-test'},
            'result': [{'value': {}}],
        },
        {
            'query': 'get_pillar',
            'kwargs': {'path': '{data,service_account_id}'},
            'result': [{'value': 'service_account_id1'}],
        },
    ] + state['metadb']['queries']
    check_task_interrupt_consistency(
        mocker,
        'clickhouse_cluster_online_resetup',
        {
            'fqdn': target_fqdn,
            'resetup_action': 'readd',
            'preserve_if_possible': True,
            'ignore_hosts': [],
            'lock_is_already_taken': True,
            'try_save_disks': True,
            'cid': 'cid-test',
            'service_account_id': 'service_account_id1',
            'resetup_from': 'someInstanceID',
        },
        state,
        ignore=[
            'instance.instance-1:stop initiated',
            'instance-1.disk-1.autodelete.False:set initiated',
            'dns.foo.db.yandex.net-AAAA-2001:db8:1::1:removed',
            'instance-1.disk.disk-2:detach initiated',
        ],
    )


def test_compute_clickhouse_cluster_readd_online_mlock_usage(mocker):
    """
    Check mlock usage
    """
    env_compute(mocker)
    target_host, target_fqdn = get_clickhouse_compute_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_clickhouse_compute_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_clickhouse_compute_host(geo='geo3'), 'baz.mdb.yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            fqdn2: host2,
            fqdn3: host3,
        },
        's3_bucket': 'test-s3-bucket',
        'zk_hosts': 'test-zk',
    }

    *_, state = checked_run_task_with_mocks(
        mocker,
        'clickhouse_cluster_create',
        args,
    )

    target_host['fqdn'] = target_fqdn
    host2['fqdn'] = fqdn2
    host3['fqdn'] = fqdn3
    state['metadb']['queries'] = [
        {
            'query': 'generic_resolve',
            'kwargs': {'cid': 'cid-test'},
            'result': [target_host, host2, host3],
        },
        {
            'query': 'get_pillar',
            'kwargs': {'cid': 'cid-test'},
            'result': [{'value': {}}],
        },
        {
            'query': 'get_pillar',
            'kwargs': {'path': '{data,service_account_id}'},
            'result': [{'value': None}],
        },
    ] + state['metadb']['queries']

    check_mlock_usage(
        mocker,
        'clickhouse_cluster_online_resetup',
        {
            'fqdn': target_fqdn,
            'resetup_action': 'readd',
            'preserve_if_possible': True,
            'ignore_hosts': [],
            'lock_is_already_taken': False,
            'try_save_disks': True,
            'cid': 'cid-test',
            'resetup_from': 'someInstanceID',
        },
        state,
    )

    check_mlock_skipped(
        mocker,
        'clickhouse_cluster_online_resetup',
        {
            'fqdn': target_fqdn,
            'resetup_action': 'readd',
            'preserve_if_possible': True,
            'ignore_hosts': [],
            'lock_is_already_taken': True,
            'try_save_disks': True,
            'cid': 'cid-test',
            'resetup_from': 'someInstanceID',
        },
        state,
    )


def test_porto_clickhouse_cluster_readd_online_mlock_usage(mocker):
    """
    Check mlock usage
    """
    env_porto(mocker)
    target_host, target_fqdn = get_clickhouse_porto_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_clickhouse_porto_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_clickhouse_porto_host(geo='geo3'), 'baz.mdb.yacloud.net'
    args = {
        'hosts': {
            'host1': target_host,
            'host2': host2,
            'host3': host3,
        },
        's3_bucket': 'test-s3-bucket',
        'zk_hosts': 'test-zk',
    }

    *_, state = checked_run_task_with_mocks(mocker, 'clickhouse_cluster_create', args)
    target_host['fqdn'] = target_fqdn
    host2['fqdn'] = fqdn2
    host3['fqdn'] = fqdn3
    state['metadb']['queries'] = [
        {
            'query': 'generic_resolve',
            'kwargs': {'cid': 'cid-test'},
            'result': [target_host, host2, host3],
        },
        {
            'query': 'get_pillar',
            'kwargs': {'cid': 'cid-test'},
            'result': [{'value': {}}],
        },
        {
            'query': 'get_pillar',
            'kwargs': {'path': '{data,service_account_id}'},
            'result': [{'value': None}],
        },
    ] + state['metadb']['queries']

    check_mlock_usage(
        mocker,
        'clickhouse_cluster_online_resetup',
        {
            'fqdn': target_fqdn,
            'resetup_action': 'readd',
            'preserve_if_possible': True,
            'ignore_hosts': [],
            'lock_is_already_taken': False,
            'try_save_disks': True,
            'cid': 'cid-test',
            'resetup_from': 'dom0.mdb.yacloud.net',
        },
        state,
    )

    check_mlock_skipped(
        mocker,
        'clickhouse_cluster_online_resetup',
        {
            'fqdn': target_fqdn,
            'resetup_action': 'readd',
            'preserve_if_possible': True,
            'ignore_hosts': [],
            'lock_is_already_taken': True,
            'try_save_disks': True,
            'cid': 'cid-test',
            'resetup_from': 'dom0.mdb.yacloud.net',
        },
        state,
    )


def test_compute_clickhouse_cluster_readd_offline_interrupt_consistency(mocker):
    env_compute(mocker)
    target_host, target_fqdn = get_clickhouse_compute_host(geo='geo1'), 'foo.mdb.yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )
    target_host['fqdn'] = target_fqdn
    state['metadb']['queries'] = [
        {
            'query': 'generic_resolve',
            'kwargs': {'cid': 'cid-test'},
            'result': [target_host],
        },
        {
            'query': 'get_pillar',
            'kwargs': {},
            'result': [{'value': {}}],
        },
        {
            'query': 'add_cluster_target_pillar',
            'kwargs': {},
            'result': [],
        },
    ] + state['metadb']['queries']
    state['internal-api-config']['backups'] = {
        'cid-test': {
            'backups': [
                {'id': 'cid-test:shard:', 'sourceShardNames': ['shard1']},
            ],
        },
    }
    check_task_interrupt_consistency(
        mocker,
        'clickhouse_cluster_offline_resetup',
        {
            'fqdn': target_fqdn,
            'resetup_action': 'restore',
            'preserve_if_possible': True,
            'ignore_hosts': [],
            'lock_is_already_taken': True,
            'try_save_disks': True,
            'cid': 'cid-test',
            'resetup_from': 'someInstanceID',
        },
        state,
        ignore=[
            'instance.instance-1:start initiated',
            'instance-1.disk-1.autodelete.False:set initiated',
            'instance-1.disk.disk-2:detach initiated',
        ],
    )


def test_porto_clickhouse_cluster_restore_offline_interrupt_consistency(mocker):
    env_porto(mocker)
    target_host, target_fqdn = get_clickhouse_porto_host(geo='geo1'), 'foo.mdb.yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )
    target_host['fqdn'] = target_fqdn
    state['metadb']['queries'] = [
        {
            'query': 'generic_resolve',
            'kwargs': {'cid': 'cid-test'},
            'result': [target_host],
        },
        {
            'query': 'get_pillar',
            'kwargs': {},
            'result': [{'value': {}}],
        },
        {
            'query': 'add_cluster_target_pillar',
            'kwargs': {},
            'result': [],
        },
    ] + state['metadb']['queries']
    state['conductor']['groups']['db_cid_test'] = {
        'id': 100,
        'name': 'db_cid_test',
        'parent_ids': [],
        'project': {
            'id': 1,
        },
    }
    state['internal-api-config']['backups'] = {
        'cid-test': {
            'backups': [
                {'id': 'cid-test:shard:', 'sourceShardNames': ['shard1']},
            ],
        },
    }
    check_task_interrupt_consistency(
        mocker,
        'clickhouse_cluster_offline_resetup',
        {
            'fqdn': target_fqdn,
            'resetup_action': 'restore',
            'preserve_if_possible': True,
            'ignore_hosts': [],
            'lock_is_already_taken': True,
            'try_save_disks': True,
            'cid': 'cid-test',
            'resetup_from': 'dom0.mdb.yacloud.net',
        },
        state,
        ignore=[
            'foo.mdb.yacloud.net:unregister initiated',
        ],
    )


def test_compute_clickhouse_offline_resetup_mlock_usage(mocker):
    """
    Check mlock usage
    """
    env_compute(mocker)
    target_host, target_fqdn = get_clickhouse_compute_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_clickhouse_compute_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_clickhouse_compute_host(geo='geo3'), 'baz.mdb..yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            fqdn2: host2,
            fqdn3: host3,
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )
    target_host['fqdn'] = target_fqdn
    host2['fqdn'] = fqdn2
    host3['fqdn'] = fqdn3
    state['metadb']['queries'] = [
        {
            'query': 'generic_resolve',
            'kwargs': {'cid': 'cid-test'},
            'result': [target_host, host2, host3],
        },
        {
            'query': 'get_pillar',
            'kwargs': {'path': '{data,service_account_id}'},
            'result': [{'value': None}],
        },
    ] + state['metadb']['queries']

    check_mlock_usage(
        mocker,
        'clickhouse_cluster_offline_resetup',
        {
            'fqdn': target_fqdn,
            'resetup_action': 'readd',
            'preserve_if_possible': True,
            'ignore_hosts': [],
            'lock_is_already_taken': False,
            'try_save_disks': True,
            'cid': 'cid-test',
            'resetup_from': 'someInstanceID',
        },
        state,
    )

    check_mlock_skipped(
        mocker,
        'clickhouse_cluster_offline_resetup',
        {
            'fqdn': target_fqdn,
            'resetup_action': 'readd',
            'preserve_if_possible': True,
            'ignore_hosts': [],
            'lock_is_already_taken': True,
            'try_save_disks': True,
            'cid': 'cid-test',
            'resetup_from': 'someInstanceID',
        },
        state,
    )


def test_porto_clickhouse_cluster_readd_offline_interrupt_consistency(mocker):
    env_porto(mocker)
    target_host, target_fqdn = get_clickhouse_porto_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_clickhouse_porto_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_clickhouse_porto_host(geo='geo3'), 'baz.mdb.yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            fqdn2: host2,
            fqdn3: host3,
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )
    target_host['fqdn'] = target_fqdn
    host2['fqdn'] = fqdn2
    host3['fqdn'] = fqdn3
    state['metadb']['queries'] = [
        {
            'query': 'generic_resolve',
            'kwargs': {'cid': 'cid-test'},
            'result': [target_host, host2, host3],
        },
        {
            'query': 'get_pillar',
            'kwargs': {'path': '{data,service_account_id}'},
            'result': [{'value': None}],
        },
    ] + state['metadb']['queries']
    check_task_interrupt_consistency(
        mocker,
        'clickhouse_cluster_offline_resetup',
        {
            'fqdn': target_fqdn,
            'resetup_action': 'readd',
            'preserve_if_possible': True,
            'ignore_hosts': [],
            'lock_is_already_taken': True,
            'try_save_disks': True,
            'cid': 'cid-test',
            'resetup_from': 'dom0.mdb.yacloud.net',
        },
        state,
        ignore=[
            'foo.mdb.yacloud.net:unregister initiated',
        ],
    )


def test_porto_clickhouse_offline_resetup_mlock_usage(mocker):
    """
    Check mlock usage
    """
    env_porto(mocker)
    target_host, target_fqdn = get_clickhouse_porto_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_clickhouse_porto_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_clickhouse_porto_host(geo='geo3'), 'baz.mdb..yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            fqdn2: host2,
            fqdn3: host3,
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )
    target_host['fqdn'] = target_fqdn
    host2['fqdn'] = fqdn2
    host3['fqdn'] = fqdn3
    state['metadb']['queries'] = [
        {
            'query': 'generic_resolve',
            'kwargs': {'cid': 'cid-test'},
            'result': [target_host, host2, host3],
        },
        {
            'query': 'get_pillar',
            'kwargs': {'path': '{data,service_account_id}'},
            'result': [{'value': None}],
        },
    ] + state['metadb']['queries']

    check_mlock_usage(
        mocker,
        'clickhouse_cluster_offline_resetup',
        {
            'fqdn': target_fqdn,
            'resetup_action': 'readd',
            'preserve_if_possible': True,
            'ignore_hosts': [],
            'lock_is_already_taken': False,
            'try_save_disks': True,
            'cid': 'cid-test',
            'resetup_from': 'dom0.mdb.yacloud.net',
        },
        state,
    )

    check_mlock_skipped(
        mocker,
        'clickhouse_cluster_online_resetup',
        {
            'fqdn': target_fqdn,
            'resetup_action': 'readd',
            'preserve_if_possible': True,
            'ignore_hosts': [],
            'lock_is_already_taken': True,
            'try_save_disks': True,
            'cid': 'cid-test',
            'resetup_from': 'dom0.mdb.yacloud.net',
        },
        state,
    )


def test_porto_clickhouse_online_resetup_transfer(mocker):
    env_porto(mocker)
    target_host, target_fqdn = get_clickhouse_porto_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_clickhouse_porto_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_clickhouse_porto_host(geo='geo3'), 'baz.mdb..yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            fqdn2: host2,
            fqdn3: host3,
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )
    target_host['fqdn'] = target_fqdn
    host2['fqdn'] = fqdn2
    host3['fqdn'] = fqdn3
    state['metadb']['queries'] = [
        {
            'query': 'generic_resolve',
            'kwargs': {'cid': 'cid-test'},
            'result': [target_host, host2, host3],
        },
        {
            'query': 'get_pillar',
            'kwargs': {'path': '{data,service_account_id}'},
            'result': [{'value': None}],
        },
    ] + state['metadb']['queries']

    state['deploy-v2']['shipments']["42"] = {'status': 'done', 'id': "42"}
    check_dbm_transfer_called(
        mocker,
        'clickhouse_cluster_online_resetup',
        {
            'fqdn': target_fqdn,
            'resetup_action': 'readd',
            'preserve_if_possible': False,
            'ignore_hosts': [],
            'lock_is_already_taken': True,
            'try_save_disks': False,
            'cid': 'cid-test',
            'resetup_from': 'geo1-1',
        },
        state,
        context={
            'update_porto_container_secrets': {'deploy': {'deploy_id': '42', 'deploy_version': 2, 'host': target_fqdn}},
        },
    )


def test_porto_clickhouse_online_resetup_no_transfer(mocker):
    env_porto(mocker)
    target_host, target_fqdn = get_clickhouse_porto_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_clickhouse_porto_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_clickhouse_porto_host(geo='geo3'), 'baz.mdb..yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            fqdn2: host2,
            fqdn3: host3,
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )
    target_host['fqdn'] = target_fqdn
    host2['fqdn'] = fqdn2
    host3['fqdn'] = fqdn3
    state['metadb']['queries'] = [
        {
            'query': 'generic_resolve',
            'kwargs': {'cid': 'cid-test'},
            'result': [target_host, host2, host3],
        },
        {
            'query': 'get_pillar',
            'kwargs': {'path': '{data,service_account_id}'},
            'result': [{'value': None}],
        },
    ] + state['metadb']['queries']

    check_dbm_without_transfer(
        mocker,
        'clickhouse_cluster_online_resetup',
        {
            'fqdn': target_fqdn,
            'resetup_action': 'readd',
            'preserve_if_possible': False,
            'ignore_hosts': [],
            'lock_is_already_taken': True,
            'try_save_disks': False,
            'cid': 'cid-test',
            'resetup_from': 'another_dom0',
        },
        state,
    )


def test_compute_clickhouse_online_resetup_recreate_vm(mocker):
    env_compute(mocker)
    target_host, target_fqdn = get_clickhouse_compute_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_clickhouse_compute_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_clickhouse_compute_host(geo='geo3'), 'baz.mdb.yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            fqdn2: host2,
            fqdn3: host3,
        },
        's3_bucket': 'test-s3-bucket',
        'zk_hosts': 'test-zk',
    }

    *_, state = checked_run_task_with_mocks(
        mocker,
        'clickhouse_cluster_create',
        args,
    )

    target_host['fqdn'] = target_fqdn
    host2['fqdn'] = fqdn2
    host3['fqdn'] = fqdn3
    state['metadb']['queries'] = [
        {
            'query': 'generic_resolve',
            'kwargs': {'cid': 'cid-test'},
            'result': [target_host, host2, host3],
        },
        {
            'query': 'get_pillar',
            'kwargs': {'cid': 'cid-test'},
            'result': [{'value': {}}],
        },
        {
            'query': 'get_pillar',
            'kwargs': {'path': '{data,service_account_id}'},
            'result': [{'value': None}],
        },
    ] + state['metadb']['queries']

    check_compute_delete_instance_called(
        mocker,
        'clickhouse_cluster_online_resetup',
        {
            'fqdn': target_fqdn,
            'resetup_action': 'readd',
            'preserve_if_possible': False,
            'ignore_hosts': [],
            'lock_is_already_taken': True,
            'try_save_disks': False,
            'cid': 'cid-test',
            'resetup_from': 'geo1-1',
        },
        state,
    )


def test_compute_clickhouse_online_resetup_preserve_vm(mocker):
    env_compute(mocker)
    target_host, target_fqdn = get_clickhouse_compute_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_clickhouse_compute_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_clickhouse_compute_host(geo='geo3'), 'baz.mdb.yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            fqdn2: host2,
            fqdn3: host3,
        },
        's3_bucket': 'test-s3-bucket',
        'zk_hosts': 'test-zk',
    }

    *_, state = checked_run_task_with_mocks(
        mocker,
        'clickhouse_cluster_create',
        args,
    )

    target_host['fqdn'] = target_fqdn
    host2['fqdn'] = fqdn2
    host3['fqdn'] = fqdn3
    state['metadb']['queries'] = [
        {
            'query': 'generic_resolve',
            'kwargs': {'cid': 'cid-test'},
            'result': [target_host, host2, host3],
        },
        {
            'query': 'get_pillar',
            'kwargs': {'cid': 'cid-test'},
            'result': [{'value': {}}],
        },
        {
            'query': 'get_pillar',
            'kwargs': {'path': '{data,service_account_id}'},
            'result': [{'value': None}],
        },
    ] + state['metadb']['queries']

    state['compute']['instances']['instance-1'] = None
    check_compute_delete_instance_not_called(
        mocker,
        'clickhouse_cluster_online_resetup',
        {
            'fqdn': target_fqdn,
            'resetup_action': 'readd',
            'preserve_if_possible': False,
            'ignore_hosts': [],
            'lock_is_already_taken': True,
            'try_save_disks': False,
            'cid': 'cid-test',
            'resetup_from': 'instance-1',
        },
        state,
    )


def test_zookeeper_node_resetup(mocker):
    env_compute(mocker)

    host, fqdn = get_clickhouse_compute_host(geo='geo2'), 'bar.mdb.yacloud.net'
    zk1, zk1_fqdn = get_zookeeper_compute_host(geo='geo1'), 'zk1.mdb.yacloud.net'
    zk2, zk2_fqdn = get_zookeeper_compute_host(geo='geo2'), 'zk2.mdb.yacloud.net'
    zk3, zk3_fqdn = get_zookeeper_compute_host(geo='geo3'), 'zk3.mdb.yacloud.net'
    args = {
        'hosts': {
            fqdn: host,
            zk1_fqdn: zk1,
            zk2_fqdn: zk2,
            zk3_fqdn: zk3,
        },
        's3_bucket': 'test-s3-bucket',
        'zk_hosts': [zk1_fqdn, zk2_fqdn, zk3_fqdn],
    }
    target_host, target_fqdn = zk1, zk1_fqdn

    *_, initial_state = checked_run_task_with_mocks(
        mocker,
        'clickhouse_cluster_create',
        args,
    )
    host['fqdn'] = fqdn
    target_host['fqdn'] = target_fqdn
    zk2['fqdn'] = zk2_fqdn
    zk3['fqdn'] = zk3_fqdn
    initial_state['metadb']['queries'] = [
        {
            'query': 'generic_resolve',
            'kwargs': {'cid': 'cid-test'},
            'result': [host, zk1, zk2, zk3],
        },
        {
            'query': 'get_pillar',
            'kwargs': {'cid': 'cid-test'},
            'result': [{'value': {}}],
        },
        {
            'query': 'get_pillar',
            'kwargs': {'path': '{data,service_account_id}'},
            'result': [{'value': None}],
        },
        {
            'query': 'get_pillar',
            'kwargs': {'path': ['data', 'zk', 'nodes']},
            'result': [{'value': {zk1_fqdn: 1, zk2_fqdn: 2, zk3_fqdn: 3}}],
        },
    ] + initial_state['metadb']['queries']

    start_state = prepare_state(initial_state)
    *_, state = checked_run_task_with_mocks(
        mocker,
        'clickhouse_cluster_online_resetup',
        {
            'fqdn': target_fqdn,
            'resetup_action': 'readd',
            'preserve_if_possible': False,
            'ignore_hosts': [],
            'lock_is_already_taken': True,
            'try_save_disks': False,
            'cid': 'cid-test',
            'resetup_from': '',
        },
        state=start_state,
    )

    assert (
        f'{target_fqdn}-state.highstate' in state["deploy-v2"]["shipments"]
    ), 'highstate for new ZK host must be executed.'

    assert (
        'components.dbaas-operations.cluster-reconfig'
        in state["deploy-v2"]["shipments"][f'{zk2_fqdn}-state.sls']["commands"][0]["arguments"]
    ), 'cluster-reconfig must be done for new zk host.'
