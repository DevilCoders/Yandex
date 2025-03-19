from cloud.mdb.dbaas_worker.internal.providers.managed_kubernetes import ManagedKubernetes
from cloud.mdb.dbaas_worker.internal.providers.load_balancer import LoadBalancer
from ...common.cluster.delete import ClusterDeleteExecutor
from ...utils import register_executor
from cloud.mdb.dbaas_worker.internal.providers.managed_postgresql import ManagedPostgresql
from ....providers.pillar import DbaasPillar
from .utils import (
    get_network_load_balancer_name,
    get_target_group_name,
)


@register_executor('metastore_cluster_delete')
class MetastoreClusterDelete(ClusterDeleteExecutor):
    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.managed_kubernetes = ManagedKubernetes(self.config, self.task, self.queue)
        self.managed_postgresql = ManagedPostgresql(self.config, self.task, self.queue)
        self.network_load_balancer = LoadBalancer(self.config, self.task, self.queue)

        self.pillar = DbaasPillar(config, task, queue)

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        pillar = self.pillar.get('cid', self.task['cid'], ['data'])
        cid = self.task['cid']

        kubernetes_cluster_id = pillar['kubernetes_cluster_id']
        postgresql_cluster_id = pillar['postgresql_cluster_id']

        self.managed_kubernetes.initialize_kubernetes_master(
            kubernetes_cluster_id,
            namespace=pillar['kubernetes_namespace_name'],
        )
        self.managed_kubernetes.kubernetes_master.namespace_absent()

        kubernetes_cluster = self.managed_kubernetes.client.get_cluster(cluster_id=kubernetes_cluster_id)

        network_balancer_name = get_network_load_balancer_name(cid)
        for network_load_balancer in self.network_load_balancer.client.list_network_load_balancers(
            folder_id=kubernetes_cluster.folder_id,
        ):
            if network_load_balancer.name == network_balancer_name:
                self.network_load_balancer.client.delete_network_load_balancer(
                    network_load_balancer_id=network_load_balancer.id,
                    wait=True,
                )
                break

        target_group_name = get_target_group_name(cid)
        for target_group in self.network_load_balancer.client.list_target_groups(
            folder_id=kubernetes_cluster.folder_id,
        ):
            if target_group.name == target_group_name:
                self.network_load_balancer.client.delete_target_group(
                    target_group_id=target_group.id,
                    wait=True,
                )
                break

        self.managed_postgresql.database_absent(cluster_id=postgresql_cluster_id, database_name=pillar['db_name'])
        self.managed_postgresql.user_absent(cluster_id=postgresql_cluster_id, user_name=pillar['db_user_name'])

        if 'kubernetes_cluster_id' in self.args:
            self.managed_kubernetes.cluster_absent(kubernetes_cluster_id=self.args['kubernetes_cluster_id'])
        elif 'node_group_id' in self.args:
            self.managed_kubernetes.node_group_absent(node_group_id=self.args['node_group_id'])
        elif 'node_group_ids' in self.args:
            for node_group_id in self.args['node_group_ids']:
                self.managed_kubernetes.node_group_absent(node_group_id=node_group_id)
        self.mlock.unlock_cluster()
