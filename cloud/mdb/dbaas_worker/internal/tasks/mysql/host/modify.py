"""
MySQL Modify host executor
"""

from ....providers.mysync import MySync
from ...common.host.modify import HostModifyExecutor
from ...utils import register_executor


@register_executor('mysql_host_modify')
class MySQLHostModify(HostModifyExecutor):
    """
    Modify mysql host
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.mysync = MySync(config, task, queue)
        self.properties = config.mysql

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        pillar = {}
        if self.args.get('ha_status_changed') or self.args.get('include-metadata'):
            pillar['include-metadata'] = True

        ha_status_changed = self.args.get('ha_status_changed', False)
        if ha_status_changed:
            self.mysync.ensure_replica(self.args['zk_hosts'], self.task['cid'], self.args['host']['fqdn'])

        if self.args.get('restart'):
            self.mysync.start_maintenance(self.args['zk_hosts'], self.task['cid'])
            pillar.update({'service-restart': True})
            self._modify_host(host=self.args['host']['fqdn'], pillar=pillar)
            self.mysync.stop_maintenance(self.args['zk_hosts'], self.task['cid'])
        else:
            self._modify_host(self.args['host']['fqdn'], pillar=pillar)

        if self.args.get('ha_status_changed'):
            # host enters or leaves HA group, we need to update all /etc/dbaas.conf
            self._update_other_hosts_metadata(self.args['host']['fqdn'])

        self.mlock.unlock_cluster()
