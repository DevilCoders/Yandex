"""
Kafka Modify cluster executor
"""

from ..utils import build_kafka_host_groups, make_cluster_dns_record
from ...common.cluster.modify import ClusterModifyExecutor
from ...common.create import BaseCreateExecutor
from ...utils import register_executor, HostGroup
from copy import deepcopy

REBALANCE_SLS = 'components.kafka_cluster.operations.topic-rebalance'


@register_executor('kafka_cluster_modify')
class KafkaClusterModify(ClusterModifyExecutor):
    """
    Modify kafka cluster
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.create_executer = BaseCreateExecutor(config, task, queue, args)

    def create_hosts(self, hosts, kafka_group: HostGroup, zk_group: HostGroup):
        if any(fqdn for fqdn in zk_group.hosts if fqdn in hosts):
            raise RuntimeError('Adding zk host for kafka is not supported')

        kafka_group_create = HostGroup(
            properties=deepcopy(kafka_group.properties),
            hosts={fqdn: host for fqdn, host in kafka_group.hosts.items() if fqdn in hosts},
        )
        kafka_group_existing = HostGroup(
            properties=deepcopy(kafka_group.properties),
            hosts={fqdn: host for fqdn, host in kafka_group.hosts.items() if fqdn not in hosts},
        )
        self.create_executer._create_cluster_service_account(kafka_group_create)
        self.create_executer._create_host_secrets(kafka_group_create)
        self.create_executer._create_host_group(kafka_group_create, revertable=True)
        self.create_executer._issue_tls(kafka_group_create)
        self.create_executer._run_operation_host_group(kafka_group_existing, 'metadata')
        self.create_executer._run_operation_host_group(zk_group, 'metadata')
        self.create_executer._highstate_host_group(kafka_group_create)
        self.create_executer._enable_monitoring(kafka_group_create)

    def rebalance_topics(self, kafka_group):
        deploys = []
        for host, opts in kafka_group.hosts.items():
            deploys.append(self._run_sls_host(host, REBALANCE_SLS, pillar={}, environment=opts['environment']))
        self.deploy_api.wait(deploys)

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        kafka_group, zk_group = build_kafka_host_groups(self.args['hosts'], self.config.kafka, self.config.zookeeper)

        if self.args.get('hosts_create'):
            self.create_hosts(self.args.get('hosts_create'), kafka_group, zk_group)
            self.rebalance_topics(kafka_group)
        elif self.args.get('update-firewall', False):
            self.create_executer._run_operation_host_group(kafka_group, 'metadata')

        if zk_group.hosts:
            self._modify_hosts(zk_group.properties, hosts=zk_group.hosts)

        order = kafka_group.hosts if self.args.get('restart') else None
        for host in kafka_group.hosts:
            self._change_host_public_ip(host)
        self._modify_hosts(kafka_group.properties, hosts=kafka_group.hosts, order=order)
        make_cluster_dns_record(self, kafka_group)
        self.mlock.unlock_cluster()
