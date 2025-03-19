"""
Redis Create cluster executor
"""
from ...common.cluster.create import ClusterCreateExecutor
from ...utils import build_host_group, register_executor

CREATE = 'redis_cluster_create'
RESTORE = 'redis_cluster_restore'
CLUSTER_INIT_SLS = 'components.redis.cluster.init_cluster'


def make_master_pillar(master: str, task_type: str, args: dict) -> dict:
    """
    Return pillar for master
    """
    pillar = {'redis-master': master, 'do-backup': False}
    if 'target-pillar-id' in args:
        pillar['target-pillar-id'] = args['target-pillar-id']
    if task_type == RESTORE:
        pillar['restore-from'] = args['restore-from']
    return pillar


def make_replica_pillar(master: str, task_type: str, args: dict) -> dict:
    """
    Return pillar for replica
    """
    pillar = {'redis-master': master}
    if task_type == RESTORE:
        if 'target-pillar-id' in args:
            pillar['target-pillar-id'] = args['target-pillar-id']
    return pillar


@register_executor(CREATE)
@register_executor(RESTORE)
class RedisClusterCreate(ClusterCreateExecutor):
    """
    Create Redis cluster in dbm/compute
    """

    def run(self):
        self.acquire_lock()
        self.update_major_version('redis')
        host_group = build_host_group(self.config.redis, self.args['hosts'])
        host_list = sorted(list(self.args['hosts'].keys()))

        master = host_list[0]
        replicas = host_list[1:]

        master_pillar = make_master_pillar(master=master, task_type=self.task['task_type'], args=self.args)
        replica_pillar = make_replica_pillar(master=master, task_type=self.task['task_type'], args=self.args)

        host_group.hosts[master]['deploy'] = {
            'pillar': master_pillar,
            'title': 'master',
        }
        for replica in replicas:
            host_group.hosts[replica]['deploy'] = {
                'pillar': replica_pillar,
                'title': 'replica',
            }

        self._create(host_group)

        if self.args.get('sharded'):
            cluster_init = [
                self._run_sls_host(
                    master, CLUSTER_INIT_SLS, pillar={}, environment=self.args['hosts'][master]['environment']
                ),
            ]
            self.deploy_api.wait(cluster_init)
        self.release_lock()
