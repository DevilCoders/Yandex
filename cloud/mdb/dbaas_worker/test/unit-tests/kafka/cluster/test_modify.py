# coding: utf-8
from queue import Queue
from mock import patch, call
from test.mocks import _get_config, get_state, eds as mock_eds
from test.tasks.utils import get_task_host
from cloud.mdb.dbaas_worker.internal.tasks.kafka.cluster.modify import KafkaClusterModify, HostGroup, REBALANCE_SLS


def mock_mlock(mocker):
    mocker.patch('cloud.mdb.dbaas_worker.internal.tasks.common.deploy.Mlock')


def mock_base_create_executor(mocker):
    return mocker.patch('cloud.mdb.dbaas_worker.internal.tasks.kafka.cluster.modify.BaseCreateExecutor').return_value


def mock_deploy_api(mocker):
    return mocker.patch('cloud.mdb.dbaas_worker.internal.tasks.common.deploy.DeployAPI').return_value


def noop(*_, **__):
    pass


def combined_sg_service_rules():
    return [
        {'direction': 'BOTH', 'ports_from': 443, 'ports_to': 443},
        {
            'direction': 'EGRESS',
            'ports_from': 443,
            'ports_to': 443,
            'protocol': 'TCP',
            'v4_cidr_blocks': ['213.180.193.243/32', '213.180.205.156/32', '93.158.157.254/32'],
            'v6_cidr_blocks': ['2a02:6b8::1d9/128', '2a02:6b8:0:3400:0:587:0:68/128', '2a02:6b8:0:3400:0:bd4:0:4/128'],
        },
        {'direction': 'BOTH', 'ports_from': 8081, 'ports_to': 8081},
        {'direction': 'BOTH', 'ports_from': 2181, 'ports_to': 2181},
        {'direction': 'BOTH', 'ports_from': 2888, 'ports_to': 2888},
        {'direction': 'BOTH', 'ports_from': 3888, 'ports_to': 3888},
        {'direction': 'BOTH', 'ports_from': 9000, 'ports_to': 9000},
        {'direction': 'BOTH', 'ports_from': 9091, 'ports_to': 9092},
        {'direction': 'BOTH', 'ports_from': 2281, 'ports_to': 2281},
    ]


def expected_config():
    expected_config = _get_config()
    expected_config.zookeeper.sg_service_rules = combined_sg_service_rules()
    expected_config.zookeeper.conductor_group_id = 'subcid-test'
    expected_config.kafka.conductor_group_id = 'subcid-test'
    expected_config.kafka.sg_service_rules = combined_sg_service_rules()

    return expected_config


def min_valid_task():
    return {
        'cid': 'cid',
        'task_id': 'task_id',
        'feature_flags': [],
        'folder_id': 'folder_id',
        'changes': [],
        'context': {},
    }


def test_run_when_vtype_is_aws_and_no_new_hosts_and_no_update_firewall_should_update_hs(mocker):
    state = get_state()
    mock_eds(mocker, state)
    mock_mlock(mocker)
    mock_base_create_executor(mocker)

    zk_host = get_task_host(vtype='aws', roles='zk')
    kafka_host = get_task_host(vtype='aws', roles='kafka')
    args = {
        'hosts': {
            'kafka_1_fqdn': kafka_host,
            'kafka_2_fqdn': kafka_host,
            'zk_1_fqdn': zk_host,
            'zk_2_fqdn': zk_host,
            'zk_3_fqdn': zk_host,
        }
    }

    modify = KafkaClusterModify(_get_config(), min_valid_task(), Queue(maxsize=10000), args)

    with patch.object(modify, '_modify_hosts', wraps=noop) as spy_modify_hosts, patch.object(
        modify, '_change_host_public_ip', wraps=noop
    ) as spy_change_host_public_ip, patch(
        'cloud.mdb.dbaas_worker.internal.tasks.kafka.cluster.modify.make_cluster_dns_record'
    ):
        modify.run()

        spy_change_host_public_ip.assert_has_calls([call('kafka_1_fqdn'), call('kafka_2_fqdn')])
        spy_modify_hosts.assert_has_calls(
            [
                call(
                    expected_config().zookeeper,
                    hosts={'zk_1_fqdn': zk_host, 'zk_2_fqdn': zk_host, 'zk_3_fqdn': zk_host},
                ),
                call(
                    expected_config().kafka,
                    hosts={'kafka_1_fqdn': kafka_host, 'kafka_2_fqdn': kafka_host},
                    order=None,
                ),
            ]
        )


def test_run_when_vtype_is_compute_and_no_new_hosts_and_no_update_firewall_should_update_hs(mocker):
    state = get_state()
    mock_eds(mocker, state)
    mock_mlock(mocker)
    mock_base_create_executor(mocker)

    args = {
        'hosts': {
            'kafka_1_fqdn': get_task_host(vtype='compute', roles='kafka'),
            'kafka_2_fqdn': get_task_host(vtype='compute', roles='kafka'),
            'zk_1_fqdn': get_task_host(vtype='compute', roles='zk'),
            'zk_2_fqdn': get_task_host(vtype='compute', roles='zk'),
            'zk_3_fqdn': get_task_host(vtype='compute', roles='zk'),
        }
    }

    modify = KafkaClusterModify(_get_config(), min_valid_task(), Queue(maxsize=10000), args)

    with patch.object(modify, '_modify_hosts', wraps=noop) as spy_modify_hosts, patch.object(
        modify, '_change_host_public_ip', wraps=noop
    ) as spy_change_host_public_ip, patch(
        'cloud.mdb.dbaas_worker.internal.tasks.kafka.cluster.modify.make_cluster_dns_record'
    ):
        modify.run()

        spy_change_host_public_ip.assert_has_calls([call('kafka_1_fqdn'), call('kafka_2_fqdn')])

        spy_modify_hosts.assert_has_calls(
            [
                call(
                    expected_config().zookeeper,
                    hosts={
                        'zk_1_fqdn': get_task_host(vtype='compute', roles='zk', managed_fqdn='zk_1_fqdn.db.yandex.net'),
                        'zk_2_fqdn': get_task_host(vtype='compute', roles='zk', managed_fqdn='zk_2_fqdn.db.yandex.net'),
                        'zk_3_fqdn': get_task_host(vtype='compute', roles='zk', managed_fqdn='zk_3_fqdn.db.yandex.net'),
                    },
                ),
                call(
                    expected_config().kafka,
                    hosts={
                        'kafka_1_fqdn': get_task_host(
                            vtype='compute', roles='kafka', managed_fqdn='kafka_1_fqdn.db.yandex.net'
                        ),
                        'kafka_2_fqdn': get_task_host(
                            vtype='compute', roles='kafka', managed_fqdn='kafka_2_fqdn.db.yandex.net'
                        ),
                    },
                    order=None,
                ),
            ]
        )


def test_run_when_vtype_is_aws_update_firewall_flag_should_update_meta_on_kafka_clusters(mocker):
    state = get_state()
    mock_eds(mocker, state)
    mock_mlock(mocker)
    mock_creator = mock_base_create_executor(mocker)

    zk_host = get_task_host(vtype='aws', roles='zk')
    kafka_host = get_task_host(vtype='aws', roles='kafka')
    args = {
        'update-firewall': True,
        'hosts': {
            'kafka_1_fqdn': kafka_host,
            'kafka_2_fqdn': kafka_host,
            'zk_1_fqdn': zk_host,
            'zk_2_fqdn': zk_host,
            'zk_3_fqdn': zk_host,
        },
    }

    modify = KafkaClusterModify(_get_config(), min_valid_task(), Queue(maxsize=10000), args)

    with patch.object(modify, '_modify_hosts', wraps=noop) as spy_modify_hosts, patch.object(
        modify, '_change_host_public_ip', wraps=noop
    ) as spy_change_host_public_ip, patch(
        'cloud.mdb.dbaas_worker.internal.tasks.kafka.cluster.modify.make_cluster_dns_record'
    ):
        modify.run()

        mock_creator._run_operation_host_group.assert_has_calls(
            [
                call(
                    HostGroup(
                        properties=expected_config().kafka,
                        hosts={'kafka_1_fqdn': kafka_host, 'kafka_2_fqdn': kafka_host},
                    ),
                    'metadata',
                )
            ]
        )
        spy_change_host_public_ip.assert_has_calls([call('kafka_1_fqdn'), call('kafka_2_fqdn')])
        spy_modify_hosts.assert_has_calls(
            [
                call(
                    expected_config().zookeeper,
                    hosts={'zk_1_fqdn': zk_host, 'zk_2_fqdn': zk_host, 'zk_3_fqdn': zk_host},
                ),
                call(
                    expected_config().kafka,
                    hosts={'kafka_1_fqdn': kafka_host, 'kafka_2_fqdn': kafka_host},
                    order=None,
                ),
            ]
        )


def test_run_when_vtype_is_aws_and_add_new_host_should_create_host_and(mocker):
    state = get_state()
    mock_eds(mocker, state)
    mock_mlock(mocker)
    mock_creator = mock_base_create_executor(mocker)
    mock_deploy_api(mocker)

    zk_host = get_task_host(vtype='aws', roles='zk', environment='dev')
    kafka_host = get_task_host(vtype='aws', roles='kafka', environment='dev')
    args = {
        'hosts_create': ['kafka_3_fqdn'],
        'hosts': {
            'kafka_1_fqdn': kafka_host,
            'kafka_2_fqdn': kafka_host,
            'kafka_3_fqdn': kafka_host,
            'zk_1_fqdn': zk_host,
            'zk_2_fqdn': zk_host,
            'zk_3_fqdn': zk_host,
        },
    }

    modify = KafkaClusterModify(_get_config(), min_valid_task(), Queue(maxsize=10000), args)

    with patch.object(modify, '_modify_hosts', wraps=noop) as spy_modify_hosts, patch.object(
        modify, '_change_host_public_ip', wraps=noop
    ) as spy_change_host_public_ip, patch.object(modify, '_run_sls_host', wraps=noop) as spy_run_sls_host, patch(
        'cloud.mdb.dbaas_worker.internal.tasks.kafka.cluster.modify.make_cluster_dns_record'
    ):
        modify.run()

        expected_kafka_hosts = {
            'kafka_1_fqdn': kafka_host,
            'kafka_2_fqdn': kafka_host,
            'kafka_3_fqdn': kafka_host,
        }
        expected_kafka_group_created = HostGroup(
            properties=expected_config().kafka,
            hosts={'kafka_3_fqdn': kafka_host},
        )
        expected_zk_hosts = {'zk_1_fqdn': zk_host, 'zk_2_fqdn': zk_host, 'zk_3_fqdn': zk_host}

        # check creating of a new host
        mock_creator._create_host_secrets.assert_called_once_with(expected_kafka_group_created)
        mock_creator._create_host_group.assert_called_once_with(expected_kafka_group_created, revertable=True)
        mock_creator._issue_tls.assert_called_once_with(expected_kafka_group_created)
        mock_creator._run_operation_host_group.assert_has_calls(
            [
                call(
                    HostGroup(
                        properties=expected_config().kafka,
                        hosts={
                            'kafka_1_fqdn': kafka_host,
                            'kafka_2_fqdn': kafka_host,
                        },
                    ),
                    'metadata',
                ),
                call(
                    HostGroup(
                        properties=expected_config().zookeeper,
                        hosts=expected_zk_hosts,
                    ),
                    'metadata',
                ),
            ]
        )

        mock_creator._highstate_host_group.assert_called_once_with(expected_kafka_group_created)
        mock_creator._enable_monitoring.assert_called_once_with(expected_kafka_group_created)

        spy_run_sls_host.assert_has_calls(
            [
                call('kafka_1_fqdn', REBALANCE_SLS, pillar={}, environment='dev'),
                call('kafka_2_fqdn', REBALANCE_SLS, pillar={}, environment='dev'),
                call('kafka_3_fqdn', REBALANCE_SLS, pillar={}, environment='dev'),
            ]
        )

        # check updating all hosts
        spy_change_host_public_ip.assert_has_calls([call('kafka_1_fqdn'), call('kafka_2_fqdn'), call('kafka_3_fqdn')])

        spy_modify_hosts.assert_has_calls(
            [
                call(expected_config().zookeeper, hosts=expected_zk_hosts),
                call(expected_config().kafka, hosts=expected_kafka_hosts, order=None),
            ]
        )
