"""
SQLServer Modify host executor
"""

from ...common.host.modify import HostModifyExecutor
from ...utils import register_executor


@register_executor('sqlserver_host_modify')
class SQLServerHostModify(HostModifyExecutor):
    """
    Modify sqlserver host
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = config.sqlserver

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        pillar = {}

        if self.args.get('include-metadata'):
            pillar['include-metadata'] = True

        self._modify_host(self.args['host']['fqdn'], pillar=pillar)

        self.mlock.unlock_cluster()
