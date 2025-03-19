"""
OpenSearch Create cluster executor
"""
from ...common.cluster.create import ClusterCreateExecutor
from ...utils import register_executor, get_managed_hostname
from ..utils import host_groups

CREATE = 'opensearch_cluster_create'
RESTORE = 'opensearch_cluster_restore'


@register_executor(CREATE)
@register_executor(RESTORE)
class OpensearchClusterCreate(ClusterCreateExecutor):
    """
    Create OpenSearch cluster
    """

    def _before_highstate(self):
        if self.args.get('restore_from'):
            bucket = self.args.get('restore_from').get('bucket')
            self.s3_bucket_access.grant_access(bucket, 'ro', 'secure_backups')

    def run(self):
        self.acquire_lock()

        hosts = self.args['hosts']
        for host in hosts:
            hosts[host]['service_account_id'] = self.args.get('service_account_id')

        master_group, data_group = host_groups(hosts, self.config.opensearch_master, self.config.opensearch_data)

        if self.task['task_type'] == RESTORE:
            # need to stop dashboards instances for restore
            for host, opts in data_group.hosts.items():
                opts['deploy'] = {
                    'pillar': {
                        'stop-dashboards-for-upgrade': True,
                        'restore_from': self.args['restore_from'],
                    }
                }
                managed_hostname = get_managed_hostname(host, opts)
                self.juggler.downtime_exists(managed_hostname, duration=self.task['timeout'], service='dashboards-ping')

        groups = [data_group]
        if master_group.hosts:
            groups = [master_group] + groups

        self._create(*groups)

        if self.task['task_type'] == RESTORE:
            # dashboards packages have been installed but the service is stopped
            for opts in data_group.hosts.values():
                opts['deploy']['pillar']['stop-dashboards-for-upgrade'] = False

            self._run_operation_host_group(data_group, 'dashboards-service')

            for host, opts in data_group.hosts.items():
                managed_hostname = get_managed_hostname(host, opts)
                self.juggler.downtime_absent(managed_hostname)

        self.release_lock()
