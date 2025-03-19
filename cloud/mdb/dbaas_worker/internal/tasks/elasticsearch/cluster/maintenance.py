"""
Elasticsearch maintenance cluster executor
"""

from ..utils import host_groups, ElasticsearchExecutorTrait
from ...common.cluster.maintenance import ClusterMaintenanceExecutor
from ...utils import register_executor
from ....providers.s3_bucket import S3Bucket
from ....providers.s3_bucket_access import S3BucketAccess


@register_executor('elasticsearch_cluster_maintenance')
class ElasticsearchClusterMaintenance(ClusterMaintenanceExecutor, ElasticsearchExecutorTrait):
    """
    Runs maintenance on a cluster.

    Currently, there are 3 main ways of using this executor:
        1. Restart a cluster - provide 'restart' flag
        2. Update cluster's TLS certificates - provide 'update_tls' flag
        3. Upgrade cluster's es patch version - provide 'run_highstate', 'restart' flags
        4. Upgrade cluster's es minor version - provide 'upgrade_to_major_version' flag
        5. Upgrade cluster's es major version - Todo: Support cluster's major version when needed
        6. You can mix flags if you want to achieve multiple goals
    """

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))

        data_group, master_group = self._get_host_groups()
        self._apply_maintenance(master_group, data_group)

        self.mlock.unlock_cluster()

    def _get_host_groups(self):
        hosts = self.args['hosts']
        for host in hosts:
            hosts[host]['service_account_id'] = self.args.get('service_account_id')

        master_group, data_group = host_groups(hosts, self.config.elasticsearch_master, self.config.elasticsearch_data)
        return data_group, master_group

    def _apply_maintenance(self, master_group, data_group):
        if self.args.get('create_cloud_bucket', False):
            self._create_cloud_bucket()

        if self.args.get('update_tls', False):
            self._update_cluster_tls(data_group, master_group)

        if self.args.get('run_highstate'):
            self._es_run_highstate(data_group, self.config.elasticsearch_data)
            self._es_run_highstate(master_group, self.config.elasticsearch_master)

        if self.args.get('restart', False):
            self._restart_cluster(master_group, data_group)

        if self.args.get('upgrade_to_major_version'):
            self._es_upgrade_to_major_version(master_group, data_group)

    def _update_cluster_tls(self, data_group, master_group):
        force_tls_certs = self.args.get('force_tls_certs', False)
        self._issue_tls(master_group, force_tls_certs=force_tls_certs)
        self._issue_tls(data_group, force_tls_certs=force_tls_certs)

        operation = 'service'
        self._run_operation_host_group(master_group, operation)
        self._run_operation_host_group(data_group, operation)

    def _restart_cluster(self, master_group, data_group):
        self._es_cluster_health(master_group, data_group)

        operation = 'service'
        pillar = {
            'service-restart': True,
        }
        self._run_operation_host_group(master_group, operation, pillar=pillar, order=master_group.hosts)
        self._run_operation_host_group(data_group, operation, pillar=pillar, order=data_group.hosts)

    def _create_cloud_bucket(self):
        s3_bucket = S3Bucket(self.config, self.task, self.queue)
        s3_bucket_access = S3BucketAccess(self.config, self.task, self.queue)

        endpoint = 'secure_backups'
        bucket = 'yandexcloud-dbaas-' + self.task['cid']

        s3_bucket.exists(bucket, endpoint)
        s3_bucket_access.creds_exist(endpoint)
        s3_bucket_access.grant_access(bucket, 'rw', endpoint)
