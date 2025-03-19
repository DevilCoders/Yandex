"""
kafka cluster modify tests
"""

from hamcrest import assert_that, has_entries, has_items

from test.mocks import checked_run_task_with_mocks, get_state
from test.tasks.kafka.utils import (
    get_kafka_porto_host,
    get_kafka_compute_host,
    get_zookeeper_porto_host,
    get_zookeeper_compute_host,
)
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency, check_alerts_synchronised


def get_instance_dict(fqdn):
    return {
        'folderId': 'compute-folder-id',
        'id': f'instance-id-{fqdn}',
        'fqdn': fqdn,
        'name': f'instance-name-{fqdn}',
        'platformId': 'compute',
        'zoneId': 'zone',
        'labels': {},
        'metadata': {},
        'networkSettings': {
            'type': 'STANDARD',
        },
        'resources': {
            'memory': 2 * 1024**3,
            'cores': 2,
            'coreFraction': 100,
            'gpus': 0,
            'sockets': 0,
            'nvmeDisks': 0,
        },
        'bootDisk': {
            'autoDelete': True,
            'deviceName': 'boot',
            'diskId': 'disk-id',
            'mode': 'READ_WRITE',
            'status': 'ATTACHED',
        },
        'secondaryDisks': [],
        'networkInterfaces': [],
        'status': 'RUNNING',
    }


def test_porto_kafka_cluster_modify_interrupt_consistency(mocker):
    """
    Check porto modify interruptions
    """
    args = {
        'hosts': {
            'host1': get_kafka_porto_host(geo='geo1'),
            'host2': get_kafka_porto_host(geo='geo2'),
            'host3': get_kafka_porto_host(geo='geo3'),
            'host4': get_zookeeper_porto_host(geo='geo1'),
            'host5': get_zookeeper_porto_host(geo='geo2'),
            'host6': get_zookeeper_porto_host(geo='geo3'),
        },
    }

    state = get_state()
    state['compute']['instances'] = {
        'host1': get_instance_dict('host1'),
        'host2': get_instance_dict('host2'),
        'host3': get_instance_dict('host3'),
    }

    *_, state = checked_run_task_with_mocks(mocker, 'kafka_cluster_create', args, state=state)

    # to cause move
    args['hosts']['host1']['platform_id'] = '2'

    # to cause in-place resize
    args['hosts']['host2']['cpu_limit'] = args['hosts']['host2']['cpu_limit'] * 2

    # to cause volume resize
    args['hosts']['host3']['space_limit'] = args['hosts']['host3']['space_limit'] * 2

    args['restart'] = True

    check_task_interrupt_consistency(
        mocker,
        'kafka_cluster_modify',
        args,
        state,
        # we skip pre-restart on already moving container
        ignore=['deployv2.components.dbaas-operations.run-pre-restart-script.host1.host1-state.sls:started'],
    )


def test_compute_kafka_cluster_modify_interrupt_consistency(mocker):
    """
    Check compute modify interruptions
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

    *_, state = checked_run_task_with_mocks(mocker, 'kafka_cluster_create', args)

    # to cause local disk resize
    args['hosts']['host1']['space_limit'] = args['hosts']['host1']['space_limit'] * 2

    # to cause network disk resize and resources change
    args['hosts']['host3']['cpu_limit'] = args['hosts']['host2']['cpu_limit'] * 2
    args['hosts']['host3']['space_limit'] = args['hosts']['host2']['space_limit'] * 2

    # to cause network disk downscale
    args['hosts']['host4']['space_limit'] = args['hosts']['host3']['space_limit'] // 2

    # to cause instance attribute change
    args['service_account_id'] = 'test-service-account2'

    args['restart'] = True

    interrupted_state = check_task_interrupt_consistency(
        mocker,
        'kafka_cluster_modify',
        args,
        state,
        # local ssd resize causes instance delete
        # restore after interruption after it leaves out some changes
        ignore=[
            'host1.disks_listed:True',
            'disk.disk-10:create initiated',
            'instance-4.disk.disk-10:attach initiated',
            'deployv2.components.dbaas-operations.run-data-move-front-script.host1.host1-state.sls:started',
            'instance-4.disk.disk-10:detach initiated',
            'instance-4.disk-7.autodelete.False:set initiated',
            'host1-disks-save-before-delete:True',
            'instance.host1:delete',
            'metadb_host.host1.vtype_id:updated',
            'instance.host1:stop initiated',
            'dns.host1.db.yandex.net-AAAA-2001:db8:1::4:removed',
        ],
    )

    for hostname in ['host1', 'host2', 'host3']:
        assert_that(
            interrupted_state['compute']['instances'].values(),
            has_items(has_entries({'fqdn': hostname, 'status': 'RUNNING'})),
        )


def test_kafka_cluster_modify_mlock_usage(mocker):
    """
    Check modify mlock usage
    """
    args = {
        'hosts': {
            'host1': get_kafka_porto_host(geo='geo1'),
            'host2': get_kafka_porto_host(geo='geo2'),
            'host3': get_kafka_porto_host(geo='geo3'),
            'host4': get_zookeeper_porto_host(geo='geo1'),
            'host5': get_zookeeper_porto_host(geo='geo2'),
            'host6': get_zookeeper_porto_host(geo='geo3'),
        },
    }
    state = get_state()
    state['compute']['instances'] = {
        'host1': get_instance_dict('host1'),
        'host2': get_instance_dict('host2'),
        'host3': get_instance_dict('host3'),
    }

    *_, state = checked_run_task_with_mocks(mocker, 'kafka_cluster_create', args, state=state)

    args['restart'] = True

    check_mlock_usage(
        mocker,
        'kafka_cluster_modify',
        args,
        state,
    )


def test_kafka_cluster_modify_alert_sync(mocker):
    """
    Check modify mlock usage
    """
    args = {
        'hosts': {
            'host1': get_kafka_porto_host(geo='geo1'),
            'host2': get_kafka_porto_host(geo='geo2'),
            'host3': get_kafka_porto_host(geo='geo3'),
            'host4': get_zookeeper_porto_host(geo='geo1'),
            'host5': get_zookeeper_porto_host(geo='geo2'),
            'host6': get_zookeeper_porto_host(geo='geo3'),
        },
    }
    state = get_state()
    state['compute']['instances'] = {
        'host1': get_instance_dict('host1'),
        'host2': get_instance_dict('host2'),
        'host3': get_instance_dict('host3'),
    }

    *_, state = checked_run_task_with_mocks(mocker, 'kafka_cluster_create', args, state=state)

    args['restart'] = True

    check_alerts_synchronised(
        mocker,
        'kafka_cluster_modify',
        args,
        state,
    )


def test_kafka_cluster_modify_alt_conductor_group(mocker):
    """
    Check that cluster correctly changes alt conductor group on modify
    """
    args = {
        'hosts': {
            'host1': get_kafka_porto_host(geo='geo1', flavor='alt-test-flavor', subcid='kafka-subcid'),
            'host2': get_kafka_porto_host(geo='geo2', flavor='alt-test-flavor', subcid='kafka-subcid'),
            'host3': get_kafka_porto_host(geo='geo3', flavor='alt-test-flavor', subcid='kafka-subcid'),
            'host4': get_zookeeper_porto_host(geo='geo1', flavor='alt-test-flavor', subcid='zk-subcid'),
            'host5': get_zookeeper_porto_host(geo='geo2', flavor='alt-test-flavor', subcid='zk-subcid'),
            'host6': get_zookeeper_porto_host(geo='geo3', flavor='alt-test-flavor', subcid='zk-subcid'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'kafka_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    assert (
        state['conductor']['groups']['db_kafka_subcid']['parent_ids'][0]
        == state['conductor']['groups']['mdb_test_altgroup']['id']
    ), 'Unexpected kafka initial group id'

    assert (
        state['conductor']['groups']['db_zk_subcid']['parent_ids'][0]
        == state['conductor']['groups']['mdb_test_altgroup']['id']
    ), 'Unexpected zk initial group id'

    for host in args['hosts'].copy():
        args['hosts'][host]['flavor'] = 'test-flavor'

    args['restart'] = True
    state['compute']['instances'] = {
        'host1': get_instance_dict('host1'),
        'host2': get_instance_dict('host2'),
        'host3': get_instance_dict('host3'),
    }

    *_, state = checked_run_task_with_mocks(mocker, 'kafka_cluster_modify', args, state=state)

    assert (
        state['conductor']['groups']['db_kafka_subcid']['parent_ids'][0]
        == state['conductor']['groups']['mdb_test_kafka']['id']
    ), 'Unexpected kafka group id after modify'

    assert (
        state['conductor']['groups']['db_zk_subcid']['parent_ids'][0]
        == state['conductor']['groups']['mdb_test_zookeeper']['id']
    ), 'Unexpected zk group id after modify'
