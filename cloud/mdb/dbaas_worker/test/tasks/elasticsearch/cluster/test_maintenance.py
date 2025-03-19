"""
Elasticsearch cluster maintenance tests
"""

from hamcrest import assert_that, has_entries, has_items

from test.mocks import checked_run_task_with_mocks
from test.tasks.elasticsearch.utils import (
    get_elasticsearch_datanode_porto_host,
    get_elasticsearch_masternode_porto_host,
    get_elasticsearch_masternode_compute_host,
    get_elasticsearch_datanode_compute_host,
)
from test.tasks.utils import (
    check_mlock_usage,
    check_rejected,
    check_task_interrupt_consistency,
)


def test_elasticsearch_cluster_maintenance_not_healthy(mocker):
    args = {
        'hosts': {
            'host1': get_elasticsearch_datanode_porto_host(geo='geo1'),
            'host2': get_elasticsearch_datanode_porto_host(geo='geo2'),
            'host3': get_elasticsearch_masternode_porto_host(geo='geo1'),
            'host4': get_elasticsearch_masternode_porto_host(geo='geo2'),
            'host5': get_elasticsearch_masternode_porto_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'elasticsearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['restart'] = True

    state['juggler']['host1']['status'] = 'CRIT'

    check_rejected(
        mocker,
        'elasticsearch_cluster_maintenance',
        args,
        state,
    )


def test_elasticsearch_cluster_maintenance_mlock_usage(mocker):
    args = {
        'hosts': {
            'host1': get_elasticsearch_datanode_porto_host(geo='geo1'),
            'host2': get_elasticsearch_datanode_porto_host(geo='geo2'),
            'host3': get_elasticsearch_masternode_porto_host(geo='geo1'),
            'host4': get_elasticsearch_masternode_porto_host(geo='geo2'),
            'host5': get_elasticsearch_masternode_porto_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'elasticsearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['restart'] = True

    check_mlock_usage(
        mocker,
        'elasticsearch_cluster_maintenance',
        args,
        state,
    )


def test_porto_elasticsearch_cluster_maintenance_interrupt_consistency(mocker):
    args = {
        'hosts': {
            'host1': get_elasticsearch_datanode_porto_host(geo='geo1'),
            'host2': get_elasticsearch_datanode_porto_host(geo='geo2'),
            'host3': get_elasticsearch_masternode_porto_host(geo='geo1'),
            'host4': get_elasticsearch_masternode_porto_host(geo='geo2'),
            'host5': get_elasticsearch_masternode_porto_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'elasticsearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    # to cause move
    args['hosts']['host1']['platform_id'] = '2'

    # to cause in-place resize
    args['hosts']['host2']['cpu_limit'] = args['hosts']['host2']['cpu_limit'] * 2

    # to cause volume resize
    args['hosts']['host3']['space_limit'] = args['hosts']['host3']['space_limit'] * 2

    args['restart'] = True

    check_task_interrupt_consistency(
        mocker,
        'elasticsearch_cluster_maintenance',
        args,
        state,
        # we skip pre-restart on already moving container
        ignore=['deployv2.components.dbaas-operations.run-pre-restart-script.host1.host1-state.sls:started'],
    )


def test_compute_elasticsearch_cluster_maintenance_interrupt_consistency(mocker):
    args = {
        'hosts': {
            'host1': get_elasticsearch_datanode_compute_host(
                geo='geo1', disk_type_id='local-ssd', space_limit=107374182400
            ),
            'host2': get_elasticsearch_datanode_compute_host(geo='geo2'),
            'host3': get_elasticsearch_masternode_compute_host(geo='geo1'),
            'host4': get_elasticsearch_masternode_compute_host(geo='geo2'),
            'host5': get_elasticsearch_masternode_compute_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'elasticsearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    # to cause local disk resize
    args['hosts']['host1']['space_limit'] = args['hosts']['host1']['space_limit'] * 2

    # to cause network disk resize and resources change
    args['hosts']['host3']['cpu_limit'] = args['hosts']['host2']['cpu_limit'] * 2
    args['hosts']['host3']['space_limit'] = args['hosts']['host2']['space_limit'] * 2

    # to cause network disk downscale
    args['hosts']['host4']['space_limit'] = args['hosts']['host3']['space_limit'] // 2

    args['restart'] = True

    interrupted_state = check_task_interrupt_consistency(
        mocker,
        'elasticsearch_cluster_maintenance',
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
            'instance.instance-4:stop initiated',
        ],
    )
    for hostname in ['host1', 'host2', 'host3', 'host4', 'host5']:
        assert_that(
            interrupted_state['compute']['instances'].values(),
            has_items(has_entries({'fqdn': hostname, 'status': 'RUNNING'})),
        )
