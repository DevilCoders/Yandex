"""
PostgreSQL cluster modify tests
"""

import re

from hamcrest import assert_that, has_entries, has_items

from test.mocks import checked_run_task_with_mocks
from test.tasks.postgresql.utils import get_postgresql_compute_host, get_postgresql_porto_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency, check_alerts_synchronised


def test_porto_postgresql_cluster_modify_interrupt_consistency(mocker):
    """
    Check porto modify interruptions
    """
    args = {
        'hosts': {
            'host1': get_postgresql_porto_host(geo='geo1'),
            'host2': get_postgresql_porto_host(geo='geo2'),
            'host3': get_postgresql_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    state['zookeeper']['contenders'] = ['host3']

    # to cause move
    args['hosts']['host1']['platform_id'] = '2'

    # to cause in-place resize
    args['hosts']['host2']['cpu_limit'] = args['hosts']['host2']['cpu_limit'] * 2

    # to cause volume resize
    args['hosts']['host3']['space_limit'] = args['hosts']['host3']['space_limit'] * 2

    args['restart'] = True
    args['zk_hosts'] = 'test-zk'

    check_task_interrupt_consistency(
        mocker,
        'postgresql_cluster_modify',
        args,
        state,
        # we skip pre-restart on already moving container
        ignore=['deployv2.components.dbaas-operations.run-pre-restart-script.host1.host1-state.sls:started'],
    )


def test_compute_postgresql_cluster_modify_interrupt_consistency(mocker):
    """
    Check compute modify interruptions
    """
    args = {
        'hosts': {
            'host1': get_postgresql_compute_host(geo='geo1', disk_type_id='local-ssd', space_limit=107374182400),
            'host2': get_postgresql_compute_host(geo='geo2'),
            'host3': get_postgresql_compute_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    state['zookeeper']['contenders'] = ['host3']

    # to cause local disk resize
    args['hosts']['host1']['space_limit'] = args['hosts']['host1']['space_limit'] * 2

    # to cause network disk resize and resources change
    args['hosts']['host2']['cpu_limit'] = args['hosts']['host2']['cpu_limit'] * 2
    args['hosts']['host2']['space_limit'] = args['hosts']['host2']['space_limit'] * 2

    # to cause network disk downscale
    args['hosts']['host3']['space_limit'] = args['hosts']['host3']['space_limit'] // 2

    args['restart'] = True
    args['zk_hosts'] = 'test-zk'

    interrupted_state = check_task_interrupt_consistency(
        mocker,
        'postgresql_cluster_modify',
        args,
        state,
        # local ssd resize and network disk downsize cause instance delete
        # restore after interruption after it leaves out some changes
        ignore=[
            'host3:unregister initiated',
            'host1:unregister initiated',
            'deployv2.components.dbaas-operations.run-pre-restart-script.host1.host1-state.sls:started',
            'instance.host1:delete',
            'dns.host1.db.yandex.net-AAAA-2001:db8:1::1:removed',
            'deployv2.components.dbaas-operations.run-pre-restart-script.host3.host3-state.sls:started',
            'instance.host3:delete',
            'dns.host3.db.yandex.net-AAAA-2001:db8:3::3:removed',
        ],
    )
    for hostname in ['host1', 'host2', 'host3']:
        assert_that(
            interrupted_state['compute']['instances'].values(),
            has_items(has_entries({'fqdn': hostname, 'status': 'RUNNING'})),
        )


def test_postgresql_cluster_modify_mlock_usage(mocker):
    """
    Check modify mlock usage
    """
    args = {
        'hosts': {
            'host1': get_postgresql_porto_host(geo='geo1'),
            'host2': get_postgresql_porto_host(geo='geo2'),
            'host3': get_postgresql_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    state['zookeeper']['contenders'] = ['host3']
    args['restart'] = True
    args['zk_hosts'] = 'test-zk'

    check_mlock_usage(
        mocker,
        'postgresql_cluster_modify',
        args,
        state,
    )


def test_postgresql_cluster_modify_alert_sync(mocker):
    """
    Check modify mlock usage
    """
    args = {
        'hosts': {
            'host1': get_postgresql_porto_host(geo='geo1'),
            'host2': get_postgresql_porto_host(geo='geo2'),
            'host3': get_postgresql_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    state['zookeeper']['contenders'] = ['host3']
    args['restart'] = True
    args['zk_hosts'] = 'test-zk'

    check_alerts_synchronised(
        mocker,
        'postgresql_cluster_modify',
        args,
        state,
    )


def _get_service_deploy_order(actions):
    """
    Returns service operation deploy order
    """
    matcher = re.compile(r'deploy\-v2\-shipment\-create\-(host[1-9])\-state\.sls')

    res = {}

    for index, action in enumerate(actions):
        match = matcher.match(action)
        if not match:
            continue
        host = match.groups()[0]
        res[host] = index

    return res


def test_postgresql_cluster_modify_restart_order_upscale(mocker):
    """
    Check modify service tasks order in upscale case
    """
    args = {
        'hosts': {
            'host1': get_postgresql_porto_host(geo='geo1'),
            'host2': get_postgresql_porto_host(geo='geo2'),
            'host3': get_postgresql_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    state['zookeeper']['contenders'] = ['host3']
    args['restart'] = True
    args['zk_hosts'] = 'test-zk'

    *_, state = checked_run_task_with_mocks(mocker, 'postgresql_cluster_modify', args, state=state)

    order = _get_service_deploy_order(state['actions'])

    for replica in ['host1', 'host2']:
        assert order['host3'] > order[replica], f'Primary (host3) service restarted before replica ({replica})'


def test_postgresql_cluster_modify_restart_order_downscale(mocker):
    """
    Check modify service tasks order in downscale case
    """
    args = {
        'hosts': {
            'host1': get_postgresql_porto_host(geo='geo1'),
            'host2': get_postgresql_porto_host(geo='geo2'),
            'host3': get_postgresql_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    state['zookeeper']['contenders'] = ['host3']
    args['restart'] = True
    args['reverse_order'] = True
    args['zk_hosts'] = 'test-zk'

    *_, state = checked_run_task_with_mocks(mocker, 'postgresql_cluster_modify', args, state=state)

    order = _get_service_deploy_order(state['actions'])

    for replica in ['host1', 'host2']:
        assert order['host3'] < order[replica], f'Primary (host3) service restarted after replica ({replica})'


def test_postgresql_cluster_modify_alt_conductor_group(mocker):
    """
    Check that cluster correctly changes alt conductor group on modify
    """
    args = {
        'hosts': {
            'host1': get_postgresql_porto_host(geo='geo1', flavor='alt-test-flavor'),
            'host2': get_postgresql_porto_host(geo='geo2', flavor='alt-test-flavor'),
            'host3': get_postgresql_porto_host(geo='geo3', flavor='alt-test-flavor'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    assert (
        state['conductor']['groups']['db_cid_test']['parent_ids'][0]
        == state['conductor']['groups']['mdb_test_altgroup']['id']
    ), 'Unexpected initial group id'

    for host in args['hosts'].copy():
        args['hosts'][host]['flavor'] = 'test-flavor'

    state['zookeeper']['contenders'] = ['host3']
    args['restart'] = True
    args['zk_hosts'] = 'test-zk'

    *_, state = checked_run_task_with_mocks(mocker, 'postgresql_cluster_modify', args, state=state)

    assert (
        state['conductor']['groups']['db_cid_test']['parent_ids'][0]
        == state['conductor']['groups']['mdb_test_postgresql']['id']
    ), 'Unexpected group id after modify'
