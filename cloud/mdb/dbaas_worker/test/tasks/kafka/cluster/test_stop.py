"""
Apache Kafka cluster stop tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.kafka.utils import get_kafka_compute_host, get_zookeeper_compute_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_compute_kafka_cluster_stop_interrupt_consistency(mocker):
    """
    Check compute stop interruptions
    """
    args = {
        'hosts': {
            'host1': get_kafka_compute_host(geo='geo1'),
            'host2': get_kafka_compute_host(geo='geo2'),
            'host3': get_kafka_compute_host(geo='geo3'),
            'host4': get_zookeeper_compute_host(geo='geo1'),
            'host5': get_zookeeper_compute_host(geo='geo2'),
            'host6': get_zookeeper_compute_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'kafka_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    check_task_interrupt_consistency(
        mocker,
        'kafka_cluster_stop',
        args,
        state,
        # dns changes are CAS-based, so if nothing resolves nothing is deleted
        ignore=[
            'dns.host1.db.yandex.net-AAAA-2001:db8:1::1:removed',
            'dns.host2.db.yandex.net-AAAA-2001:db8:2::2:removed',
            'dns.host3.db.yandex.net-AAAA-2001:db8:3::3:removed',
            'dns.host4.db.yandex.net-AAAA-2001:db8:1::4:removed',
            'dns.host5.db.yandex.net-AAAA-2001:db8:2::5:removed',
            'dns.host6.db.yandex.net-AAAA-2001:db8:3::6:removed',
            'compute.instance.instance-4.STOPPED:wait ok',
            'compute.instance.instance-1.STOPPED:wait ok',
            'instance.instance-1:stop initiated',
            'instance.instance-4:stop initiated',
            'instance.instance-2:stop initiated',
            'compute.instance.instance-6.STOPPED:wait ok',
            'instance.instance-6:stop initiated',
            'instance.instance-3:stop initiated',
            'compute.instance.instance-3.STOPPED:wait ok',
            'compute.instance.instance-5.STOPPED:wait ok',
            'compute.instance.instance-2.STOPPED:wait ok',
            'instance.instance-5:stop initiated',
        ],
    )


def test_kafka_cluster_stop_mlock_usage(mocker):
    """
    Check stop mlock usage
    """
    args = {
        'hosts': {
            'host1': get_kafka_compute_host(geo='geo1'),
            'host2': get_kafka_compute_host(geo='geo2'),
            'host3': get_kafka_compute_host(geo='geo3'),
            'host4': get_zookeeper_compute_host(geo='geo1'),
            'host5': get_zookeeper_compute_host(geo='geo2'),
            'host6': get_zookeeper_compute_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'kafka_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    check_mlock_usage(
        mocker,
        'kafka_cluster_stop',
        args,
        state,
    )
