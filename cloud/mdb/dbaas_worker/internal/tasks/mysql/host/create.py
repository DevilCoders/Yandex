"""
MySQL  Create host executor
"""

from ...common.host.create import HostCreateExecutor
from ...utils import register_executor


@register_executor('mysql_host_create')
class MySQLHostCreate(HostCreateExecutor):
    """
    Create MySQL host in dbm or compute
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = self.config.mysql

    def run(self):
        self.acquire_lock()
        self._create_host()
        self.release_lock()
