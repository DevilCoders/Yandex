"""
Redis cluster maintenance tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.redis.utils import get_redis_compute_host, get_redis_porto_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_porto_redis_cluster_maintenance_interrupt_consistency(mocker):
    """
    Check porto maintenance interruptions
    """
    args = {
        'hosts': {
            'host1': get_redis_porto_host(geo='geo1'),
            'host2': get_redis_porto_host(geo='geo2'),
            'host3': get_redis_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'redis_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    # to cause move
    args['hosts']['host1']['platform_id'] = '2'

    # to cause in-place resize
    args['hosts']['host2']['cpu_limit'] = args['hosts']['host2']['cpu_limit'] * 2

    # to cause volume resize
    args['hosts']['host3']['space_limit'] = args['hosts']['host3']['space_limit'] * 2

    args['restart'] = True

    check_task_interrupt_consistency(
        mocker,
        'redis_cluster_maintenance',
        args,
        state,
        # we skip pre-restart on already moving container
        ignore=['deployv2.components.dbaas-operations.run-pre-restart-script.host1.host1-state.sls:started'],
    )


def test_compute_redis_cluster_maintenance_interrupt_consistency(mocker):
    """
    Check compute maintenance interruptions
    """
    args = {
        'hosts': {
            'host1': get_redis_compute_host(geo='geo1', disk_type_id='local-ssd', space_limit=107374182400),
            'host2': get_redis_compute_host(geo='geo2'),
            'host3': get_redis_compute_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'redis_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    # to cause local disk resize
    args['hosts']['host1']['space_limit'] = args['hosts']['host1']['space_limit'] * 2

    # to cause network disk resize and resources change
    args['hosts']['host2']['cpu_limit'] = args['hosts']['host2']['cpu_limit'] * 2
    args['hosts']['host2']['space_limit'] = args['hosts']['host2']['space_limit'] * 2

    # to cause network disk downscale
    args['hosts']['host3']['space_limit'] = args['hosts']['host3']['space_limit'] // 2

    args['restart'] = True

    check_task_interrupt_consistency(
        mocker,
        'redis_cluster_maintenance',
        args,
        state,
        # local ssd resize and network disk downsize cause instance delete
        # restore after interruption after it leaves out some changes
        ignore=[
            'deployv2.components.dbaas-operations.run-pre-restart-script.host1.host1-state.sls:started',
            'instance.host1:delete',
            'dns.host1.db.yandex.net-AAAA-2001:db8:1::1:removed',
            'deployv2.components.dbaas-operations.run-pre-restart-script.host3.host3-state.sls:started',
            'instance.host3:delete',
            'dns.host3.db.yandex.net-AAAA-2001:db8:3::3:removed',
            'host3:unregister initiated',
            'host1:unregister initiated',
        ],
    )


def test_redis_cluster_maintenance_mlock_usage(mocker):
    """
    Check maintenance mlock usage
    """
    args = {
        'hosts': {
            'host1': get_redis_porto_host(geo='geo1'),
            'host2': get_redis_porto_host(geo='geo2'),
            'host3': get_redis_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'redis_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['restart'] = True

    check_mlock_usage(
        mocker,
        'redis_cluster_maintenance',
        args,
        state,
    )
