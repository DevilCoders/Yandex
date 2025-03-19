"""
PostgreSQL Create host executor
"""

from ...common.host.create import HostCreateExecutor
from ...utils import register_executor


@register_executor('postgresql_host_create')
class PostgreSQLHostCreate(HostCreateExecutor):
    """
    Create postgresql host in dbm or compute
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = self.config.postgresql
