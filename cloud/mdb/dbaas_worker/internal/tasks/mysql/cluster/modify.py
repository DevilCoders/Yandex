"""
MySQL Modify cluster executor
"""

from ....providers.mysync import MySync
from ....utils import get_first_key
from ...common.cluster.modify import ClusterModifyExecutor
from ...utils import register_executor


@register_executor('mysql_cluster_modify')
class MySQLClusterModify(ClusterModifyExecutor):
    """
    Modify mysql cluster
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.mysync = MySync(self.config, self.task, self.queue)

    def get_order(self, _):
        """
        Dynamic order resolver with mysync
        """
        if len(self.args['hosts']) == 1:
            return [get_first_key(self.args['hosts'])]
        master = self.mysync.get_master(self.args['zk_hosts'], self.task['cid'])

        replicas = [x for x in self.args['hosts'] if x != master]

        return replicas + [master]

    def run(self):
        """
        Run modify on mysql hosts
        """
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        if self.args.get('restart'):
            self.mysync.start_maintenance(self.args['zk_hosts'], self.task['cid'])
            self._modify_hosts(self.config.mysql, order=self.get_order)
            self.mysync.stop_maintenance(self.args['zk_hosts'], self.task['cid'])
        else:
            self._modify_hosts(self.config.mysql, order=self.get_order)
        self.mlock.unlock_cluster()
