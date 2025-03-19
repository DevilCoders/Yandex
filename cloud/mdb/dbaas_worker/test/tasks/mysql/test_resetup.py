from cloud.mdb.dbaas_worker.internal.environment import EnvironmentName
from test.tasks.mysql.utils import get_mysql_compute_host, get_mysql_porto_host
from test.tasks.utils import (
    checked_run_task_with_mocks,
    check_task_interrupt_consistency,
    check_mlock_usage,
    check_mlock_skipped,
)


def set_env(mocker, env: EnvironmentName):
    get_env_name = mocker.patch('cloud.mdb.dbaas_worker.internal.tasks.common.resetup_common.get_env_name_from_config')
    get_env_name.return_value = env


def env_porto(mocker):
    set_env(mocker, EnvironmentName.PORTO)


def env_compute(mocker):
    set_env(mocker, EnvironmentName.COMPUTE)


def test_porto_mysql_cluster_readd_online_interrupt_consistency(mocker):
    """
    Check porto readd interruptions
    """
    env_porto(mocker)
    target_host, target_fqdn = get_mysql_porto_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_mysql_porto_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_mysql_porto_host(geo='geo3'), 'baz.mdb.yacloudresetup_common.py.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            fqdn2: host2,
            fqdn3: host3,
        },
        'zk_hosts': 'test-zk',
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mysql_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))
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
    ] + state['metadb']['queries']
    check_task_interrupt_consistency(
        mocker,
        'mysql_cluster_online_resetup',
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


def test_porto_mysql_cluster_restore_online_interrupt_consistency(mocker):
    """
    Check porto restore interruptions
    """
    env_porto(mocker)
    target_host, target_fqdn = get_mysql_porto_host(geo='geo1'), 'foo.mdb.yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
        },
        'zk_hosts': 'test-zk',
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mysql_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))
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
    check_task_interrupt_consistency(
        mocker,
        'mysql_cluster_online_resetup',
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


def test_compute_mysql_cluster_restore_online_interrupt_consistency(mocker):
    """
    Check compute restore interruptions
    """
    env_compute(mocker)
    target_host, target_fqdn = get_mysql_compute_host(geo='geo1'), 'foo.mdb.yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
        },
        's3_bucket': 'test-s3-bucket',
        'zk_hosts': 'test-zk',
    }

    *_, state = checked_run_task_with_mocks(
        mocker,
        'mysql_cluster_create',
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
        {
            'query': 'get_pillar',
            'kwargs': {'cid': 'cid-test'},
            'result': [{'value': {}}],
        },
    ] + state['metadb']['queries']
    check_task_interrupt_consistency(
        mocker,
        'mysql_cluster_online_resetup',
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
        ],
    )


def test_compute_mysql_cluster_readd_online_interrupt_consistency(mocker):
    """
    Check compute readd interruptions
    """
    env_compute(mocker)
    target_host, target_fqdn = get_mysql_compute_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_mysql_compute_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_mysql_compute_host(geo='geo3'), 'baz.mdb.yacloud.net'
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
        'mysql_cluster_create',
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
    ] + state['metadb']['queries']
    check_task_interrupt_consistency(
        mocker,
        'mysql_cluster_online_resetup',
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
        ignore=[
            'dns.foo.db.yandex.net-AAAA-2001:db8:1::1:removed',
            'instance-4.disk-7.autodelete.False:set initiated',
            'instance-4.disk.disk-8:detach initiated',
            'instance-1.disk-1.autodelete.False:set initiated',
            'instance-1.disk.disk-2:detach initiated',
            'instance.foo.mdb.yacloud.net:start initiated',
            'foo.mdb.yacloud.net:unregister initiated',
            'instance.instance-4:stop initiated',
            'instance.instance-1:stop initiated',
        ],
    )


def test_compute_mysql_cluster_readd_online_mlock_usage(mocker):
    """
    Check mlock usage
    """
    env_compute(mocker)
    target_host, target_fqdn = get_mysql_compute_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_mysql_compute_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_mysql_compute_host(geo='geo3'), 'baz.mdb.yacloud.net'
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
        'mysql_cluster_create',
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
    ] + state['metadb']['queries']

    check_mlock_usage(
        mocker,
        'mysql_cluster_online_resetup',
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
        'mysql_cluster_online_resetup',
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


def test_porto_mysql_cluster_readd_online_mlock_usage(mocker):
    """
    Check mlock usage
    """
    env_porto(mocker)
    target_host, target_fqdn = get_mysql_porto_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_mysql_porto_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_mysql_porto_host(geo='geo3'), 'baz.mdb.yacloudresetup_common.py.net'
    args = {
        'hosts': {
            'host1': target_host,
            'host2': host2,
            'host3': host3,
        },
        's3_bucket': 'test-s3-bucket',
        'zk_hosts': 'test-zk',
    }

    *_, state = checked_run_task_with_mocks(mocker, 'mysql_cluster_create', args)
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
    ] + state['metadb']['queries']

    check_mlock_usage(
        mocker,
        'mysql_cluster_online_resetup',
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
        'mysql_cluster_online_resetup',
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


def test_porto_mysql_cluster_readd_offline_interrupt_consistency(mocker):
    env_porto(mocker)
    target_host, target_fqdn = get_mysql_porto_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_mysql_porto_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_mysql_porto_host(geo='geo3'), 'baz.mdb.yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            fqdn2: host2,
            fqdn3: host3,
        },
        'zk_hosts': 'test-zk',
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mysql_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))
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
    ] + state['metadb']['queries']
    check_task_interrupt_consistency(
        mocker,
        'mysql_cluster_offline_resetup',
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


def test_compute_mysql_cluster_readd_offline_interrupt_consistency(mocker):
    env_compute(mocker)
    target_host, target_fqdn = get_mysql_compute_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_mysql_compute_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_mysql_compute_host(geo='geo3'), 'baz.mdb.yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            fqdn2: host2,
            fqdn3: host3,
        },
        'zk_hosts': 'test-zk',
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mysql_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))
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
    check_task_interrupt_consistency(
        mocker,
        'mysql_cluster_offline_resetup',
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
        ignore=[
            'drop_deploy_data:finished',
            'dns.baz.db.yandex.net-AAAA-2001:db8:3::3:created',
            'dns.foo.db.yandex.net-AAAA-2001:db8:1::4:removed',
            'deployv2.components.dbaas-operations.metadata.bar.mdb.yacloud.net.bar.mdb.yacloud.net-state.sls:started',
            'conductor_host.baz.db.yandex.net:created',
            'dns.bar.db.yandex.net-AAAA-2001:db8:2::2:removed',
            'compute.instance.instance-3.RUNNING:wait ok',
            'compute.instance.instance-2.RUNNING:wait ok',
            'instance.instance-2:start initiated',
            'instance.instance-3:start initiated',
            'instance.instance-4:start initiated',
            'dns.foo.db.yandex.net-AAAA-2001:db8:1::1:removed',
            'instance-1.disk-1.autodelete.False:set initiated',
            'instance.instance-1:stop initiated',
            'instance-1.disk.disk-2:detach initiated',
        ],
    )


def test_porto_mysql_cluster_restore_offline_interrupt_consistency(mocker):
    env_porto(mocker)
    target_host, target_fqdn = get_mysql_porto_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_mysql_porto_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_mysql_porto_host(geo='geo3'), 'baz.mdb..yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            fqdn2: host2,
            fqdn3: host3,
        },
        'zk_hosts': 'test-zk',
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mysql_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))
    target_host['fqdn'] = target_fqdn
    host2['fqdn'] = fqdn2
    host3['fqdn'] = fqdn3
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
    check_task_interrupt_consistency(
        mocker,
        'mysql_cluster_offline_resetup',
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


def test_compute_mysql_offline_resetup_mlock_usage(mocker):
    """
    Check mlock usage
    """
    env_compute(mocker)
    target_host, target_fqdn = get_mysql_compute_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_mysql_compute_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_mysql_compute_host(geo='geo3'), 'baz.mdb.yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            fqdn2: host2,
            fqdn3: host3,
        },
        'zk_hosts': 'test-zk',
    }

    *_, state = checked_run_task_with_mocks(mocker, 'mysql_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))
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
    ] + state['metadb']['queries']

    check_mlock_usage(
        mocker,
        'mysql_cluster_offline_resetup',
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
        'mysql_cluster_offline_resetup',
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
