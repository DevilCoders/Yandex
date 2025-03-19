"""
MySQL User delete executor
"""
from .....providers.mysync import MySync
from ....common.cluster.user.delete import UserDeleteExecutor
from ....utils import register_executor


@register_executor('mysql_user_delete')
class MySQLUserDelete(UserDeleteExecutor):
    """
    Delete mysql user
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.mysync = MySync(self.config, self.task, self.queue)

    def hosts_to_run(self):
        """
        Returns list of host on which quick operation should be executed
        """
        master = self.mysync.get_master(self.args['zk_hosts'], self.task['cid'])
        return [master]
