from cloud.mdb.dbaas_worker.internal.environment import EnvironmentName
from test.tasks.postgresql.utils import get_postgresql_compute_host, get_postgresql_porto_host
from test.tasks.utils import (
    checked_run_task_with_mocks,
    check_task_interrupt_consistency,
    check_mlock_usage,
    check_mlock_skipped,
    check_dbm_transfer_called,
    check_dbm_without_transfer,
    check_compute_delete_instance_called,
    check_compute_delete_instance_not_called,
)


def set_env(mocker, env: EnvironmentName):
    get_env_name = mocker.patch('cloud.mdb.dbaas_worker.internal.tasks.common.resetup_common.get_env_name_from_config')
    get_env_name.return_value = env


def env_porto(mocker):
    set_env(mocker, EnvironmentName.PORTO)


def env_compute(mocker):
    set_env(mocker, EnvironmentName.COMPUTE)


def test_porto_postgresql_cluster_readd_online_interrupt_consistency(mocker):
    """
    Check porto readd interruptions
    """
    env_porto(mocker)
    target_host, target_fqdn = get_postgresql_porto_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_postgresql_porto_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_postgresql_porto_host(geo='geo3'), 'baz.mdb..yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            fqdn2: host2,
            fqdn3: host3,
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )
    target_host['fqdn'] = target_fqdn
    host2['fqdn'] = fqdn2
    host3['fqdn'] = fqdn3
    state['metadb']['queries'].append(
        {
            'query': 'generic_resolve',
            'kwargs': {'cid': 'cid-test'},
            'result': [target_host, host2, host3],
        },
    )
    check_task_interrupt_consistency(
        mocker,
        'postgresql_cluster_online_resetup',
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


def test_porto_postgresql_cluster_restore_online_interrupt_consistency(mocker):
    """
    Check porto restore interruptions
    """
    env_porto(mocker)
    target_host, target_fqdn = get_postgresql_porto_host(geo='geo1'), 'foo.mdb.yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
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
    check_task_interrupt_consistency(
        mocker,
        'postgresql_cluster_online_resetup',
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


def test_compute_postgresql_cluster_restore_online_interrupt_consistency(mocker):
    """
    Check compute restore interruptions
    """
    env_compute(mocker)
    target_host, target_fqdn = get_postgresql_porto_host(geo='geo1'), 'foo.mdb.yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
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
    check_task_interrupt_consistency(
        mocker,
        'postgresql_cluster_online_resetup',
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


def test_compute_postgresql_cluster_readd_online_interrupt_consistency(mocker):
    """
    Check compute readd interruptions
    """
    env_compute(mocker)
    target_host, target_fqdn = get_postgresql_compute_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_postgresql_compute_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_postgresql_compute_host(geo='geo3'), 'baz.mdb.yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            fqdn2: host2,
            fqdn3: host3,
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    target_host['fqdn'] = target_fqdn
    host2['fqdn'] = fqdn2
    host3['fqdn'] = fqdn3
    state['metadb']['queries'].append(
        {
            'query': 'generic_resolve',
            'kwargs': {'cid': 'cid-test'},
            'result': [target_host, host2, host3],
        },
    )
    check_task_interrupt_consistency(
        mocker,
        'postgresql_cluster_online_resetup',
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
            'instance.instance-1:stop initiated',
            'instance-1.disk.disk-2:detach initiated',
            'instance-1.disk-1.autodelete.False:set initiated',
            'dns.foo.db.yandex.net-AAAA-2001:db8:1::1:removed',
            'instance.instance-1:start initiated',
        ],
    )


def test_compute_postgresql_cluster_readd_online_mlock_usage(mocker):
    """
    Check mlock usage
    """
    env_compute(mocker)
    target_host, target_fqdn = get_postgresql_compute_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_postgresql_compute_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_postgresql_compute_host(geo='geo3'), 'baz.mdb.yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            fqdn2: host2,
            fqdn3: host3,
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    target_host['fqdn'] = target_fqdn
    host2['fqdn'] = fqdn2
    host3['fqdn'] = fqdn3
    state['metadb']['queries'].append(
        {
            'query': 'generic_resolve',
            'kwargs': {'cid': 'cid-test'},
            'result': [target_host, host2, host3],
        },
    )

    check_mlock_usage(
        mocker,
        'postgresql_cluster_online_resetup',
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
        'postgresql_cluster_online_resetup',
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


def test_postgresql_online_resetup_mlock_usage(mocker):
    """
    Check mlock usage
    """
    env_porto(mocker)
    target_host, target_fqdn = get_postgresql_porto_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_postgresql_porto_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_postgresql_porto_host(geo='geo3'), 'baz.mdb..yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            fqdn2: host2,
            fqdn3: host3,
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )
    target_host['fqdn'] = target_fqdn
    host2['fqdn'] = fqdn2
    host3['fqdn'] = fqdn3
    state['metadb']['queries'].append(
        {
            'query': 'generic_resolve',
            'kwargs': {'cid': 'cid-test'},
            'result': [target_host, host2, host3],
        },
    )

    check_mlock_usage(
        mocker,
        'postgresql_cluster_online_resetup',
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
        'postgresql_cluster_online_resetup',
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


def test_porto_postgresql_offline_resetup_mlock_usage(mocker):
    """
    Check mlock usage
    """
    env_porto(mocker)
    target_host, target_fqdn = get_postgresql_porto_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_postgresql_porto_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_postgresql_porto_host(geo='geo3'), 'baz.mdb..yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            fqdn2: host2,
            fqdn3: host3,
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )
    # state = get_state()
    target_host['fqdn'] = target_fqdn
    host2['fqdn'] = fqdn2
    host3['fqdn'] = fqdn3
    state['metadb']['queries'].append(
        {
            'query': 'generic_resolve',
            'kwargs': {'cid': 'cid-test'},
            'result': [target_host, host2, host3],
        },
    )

    check_mlock_usage(
        mocker,
        'postgresql_cluster_offline_resetup',
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
        'postgresql_cluster_online_resetup',
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


def test_porto_postgresql_cluster_readd_offline_interrupt_consistency(mocker):
    env_porto(mocker)
    target_host, target_fqdn = get_postgresql_porto_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_postgresql_porto_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_postgresql_porto_host(geo='geo3'), 'baz.mdb.yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            fqdn2: host2,
            fqdn3: host3,
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )
    target_host['fqdn'] = target_fqdn
    host2['fqdn'] = fqdn2
    host3['fqdn'] = fqdn3
    state['metadb']['queries'].append(
        {
            'query': 'generic_resolve',
            'kwargs': {'cid': 'cid-test'},
            'result': [target_host, host2, host3],
        },
    )
    check_task_interrupt_consistency(
        mocker,
        'postgresql_cluster_offline_resetup',
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


def test_compute_postgresql_cluster_readd_offline_interrupt_consistency(mocker):
    env_compute(mocker)
    target_host, target_fqdn = get_postgresql_compute_host(geo='geo1'), 'foo.mdb.yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
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
    check_task_interrupt_consistency(
        mocker,
        'postgresql_cluster_offline_resetup',
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
            'instance-1.disk.disk-2:detach initiated',
            'instance-1.disk-1.autodelete.False:set initiated',
        ],
    )


def test_porto_postgresql_cluster_restore_offline_interrupt_consistency(mocker):
    env_porto(mocker)
    target_host, target_fqdn = get_postgresql_porto_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_postgresql_porto_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_postgresql_porto_host(geo='geo3'), 'baz.mdb..yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            fqdn2: host2,
            fqdn3: host3,
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )
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
        'postgresql_cluster_offline_resetup',
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


def test_compute_postgresql_offline_resetup_mlock_usage(mocker):
    """
    Check mlock usage
    """
    env_compute(mocker)
    target_host, target_fqdn = get_postgresql_compute_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_postgresql_compute_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_postgresql_compute_host(geo='geo3'), 'baz.mdb..yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            fqdn2: host2,
            fqdn3: host3,
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )
    target_host['fqdn'] = target_fqdn
    host2['fqdn'] = fqdn2
    host3['fqdn'] = fqdn3
    state['metadb']['queries'].append(
        {
            'query': 'generic_resolve',
            'kwargs': {'cid': 'cid-test'},
            'result': [target_host, host2, host3],
        },
    )

    check_mlock_usage(
        mocker,
        'postgresql_cluster_offline_resetup',
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
        'postgresql_cluster_offline_resetup',
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


def test_porto_postgresql_online_resetup_transfer(mocker):
    env_porto(mocker)
    target_host, target_fqdn = get_postgresql_porto_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_postgresql_porto_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_postgresql_porto_host(geo='geo3'), 'baz.mdb..yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            fqdn2: host2,
            fqdn3: host3,
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )
    target_host['fqdn'] = target_fqdn
    host2['fqdn'] = fqdn2
    host3['fqdn'] = fqdn3
    state['metadb']['queries'].append(
        {
            'query': 'generic_resolve',
            'kwargs': {'cid': 'cid-test'},
            'result': [target_host, host2, host3],
        },
    )

    state['deploy-v2']['shipments']["42"] = {'status': 'done', 'id': "42"}
    check_dbm_transfer_called(
        mocker,
        'postgresql_cluster_online_resetup',
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


def test_porto_postgresql_online_resetup_no_transfer(mocker):
    env_porto(mocker)
    target_host, target_fqdn = get_postgresql_porto_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_postgresql_porto_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_postgresql_porto_host(geo='geo3'), 'baz.mdb..yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            fqdn2: host2,
            fqdn3: host3,
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )
    target_host['fqdn'] = target_fqdn
    host2['fqdn'] = fqdn2
    host3['fqdn'] = fqdn3
    state['metadb']['queries'].append(
        {
            'query': 'generic_resolve',
            'kwargs': {'cid': 'cid-test'},
            'result': [target_host, host2, host3],
        },
    )

    check_dbm_without_transfer(
        mocker,
        'postgresql_cluster_online_resetup',
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


def test_compute_postgresql_online_resetup_recreate_vm(mocker):
    env_compute(mocker)
    target_host, target_fqdn = get_postgresql_compute_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_postgresql_compute_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_postgresql_compute_host(geo='geo3'), 'baz.mdb.yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            fqdn2: host2,
            fqdn3: host3,
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    target_host['fqdn'] = target_fqdn
    host2['fqdn'] = fqdn2
    host3['fqdn'] = fqdn3
    state['metadb']['queries'].append(
        {
            'query': 'generic_resolve',
            'kwargs': {'cid': 'cid-test'},
            'result': [target_host, host2, host3],
        },
    )

    check_compute_delete_instance_called(
        mocker,
        'postgresql_cluster_online_resetup',
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


def test_compute_postgresql_online_resetup_preserve_vm(mocker):
    env_compute(mocker)
    target_host, target_fqdn = get_postgresql_compute_host(geo='geo1'), 'foo.mdb.yacloud.net'
    host2, fqdn2 = get_postgresql_compute_host(geo='geo2'), 'bar.mdb.yacloud.net'
    host3, fqdn3 = get_postgresql_compute_host(geo='geo3'), 'baz.mdb.yacloud.net'
    args = {
        'hosts': {
            target_fqdn: target_host,
            fqdn2: host2,
            fqdn3: host3,
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    target_host['fqdn'] = target_fqdn
    host2['fqdn'] = fqdn2
    host3['fqdn'] = fqdn3
    state['metadb']['queries'].append(
        {
            'query': 'generic_resolve',
            'kwargs': {'cid': 'cid-test'},
            'result': [target_host, host2, host3],
        },
    )

    state['compute']['instances']['instance-1'] = None
    check_compute_delete_instance_not_called(
        mocker,
        'postgresql_cluster_online_resetup',
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
