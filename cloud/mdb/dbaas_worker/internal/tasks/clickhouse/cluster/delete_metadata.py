"""
ClickHouse Delete Metadata cluster executor
"""
from ....providers.clickhouse.cloud_storage import CloudStorage
from ....providers.aws.iam import DOUBLE_CLOUD_CLUSTERS_IAM_PATH
from ...common.cluster.delete_metadata import ClusterDeleteMetadataExecutor
from ...utils import register_executor, build_host_group_from_list
from ..utils import classify_host_map, build_host_group, get_managed_hostname


@register_executor('clickhouse_cluster_delete_metadata')
class ClickhouseClusterDeleteMetadata(ClusterDeleteMetadataExecutor):
    """
    Delete clickhouse cluster metadata
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.cloud_storage = CloudStorage(self.config, self.task, self.queue, self.args)
        self.properties = self.config.clickhouse

    def run(self):
        hosts = self.args['hosts']
        ch_hosts, zk_hosts = classify_host_map(hosts)

        if self._is_compute(ch_hosts) and self.cloud_storage.has_cloud_storage_bucket():
            self.cloud_storage.service_account_absent()

        if self._is_aws(ch_hosts):
            self.aws_iam.delete_user('cluster-user-' + self.task['cid'], DOUBLE_CLOUD_CLUSTERS_IAM_PATH)

        self._delete_hosts()

        if zk_hosts:
            for opts in zk_hosts.values():
                if 'subcid' in opts:
                    zk_subcid = opts['subcid']
                    break
            self.conductor.group_absent(zk_subcid)
            zk_host_group = build_host_group(self.config.zookeeper, zk_hosts)
            for host, opts in zk_host_group.hosts.items():
                managed_hostname = get_managed_hostname(host, opts)
                self.eds_api.host_unregister(managed_hostname)
        else:
            shared_zk_hosts = self.args.get('zk_hosts')  # 'zk01:2181,zk02:2181,zk03:2181'
            if shared_zk_hosts:
                nodes = []
                for prefix in ['clickhouse', 'ch-backup']:
                    nodes.append('/{prefix}/{cid}'.format(prefix=prefix, cid=self.task['cid']))

                shared_zk_host_list = []
                for host in shared_zk_hosts.split(','):
                    shared_zk_host_list.append(host.split(':')[0])

                #  self.properties from clickhouse cluster, but properties are not used in _run_operation_host_group
                shared_zk_host_group = build_host_group_from_list(self.properties, shared_zk_host_list)
                self._run_operation_host_group(
                    shared_zk_host_group,
                    'cleanup',
                    title='post-clickhouse-cluster-delete-cleanup',
                    pillar={
                        'zookeeper-nodes': {
                            'removed-nodes': nodes,
                        },
                    },
                )

        for opts in ch_hosts.values():
            if 'subcid' in opts:
                ch_subcid = opts['subcid']
                break
        self.conductor.group_absent(ch_subcid)
        ch_host_group = build_host_group(self.config.clickhouse, ch_hosts)
        for host, opts in ch_host_group.hosts.items():
            managed_hostname = get_managed_hostname(host, opts)
            self.eds_api.host_unregister(managed_hostname)
        self._delete_encryption_stuff(ch_host_group)
        self._delete_cluster_service_account(ch_host_group)
