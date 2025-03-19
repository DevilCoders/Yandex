"""
MongoDB Create host executor
"""

from ....utils import filter_host_map, get_first_value, get_host_role
from ...common.host.create import HostCreateExecutor
from ...utils import build_host_group, register_executor
from ..utils import ROLE_HOST_TYPE


@register_executor('mongodb_host_create')
class MongoDBHostCreate(HostCreateExecutor):
    """
    Create mongodb host in dbm or compute
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        hosts = filter_host_map(
            self.args['hosts'],
            subcid=self.args['subcid'],
            shard_id=self.args['shard_id'],
        )

        host_role = get_host_role(get_first_value(hosts), ROLE_HOST_TYPE)
        if len(hosts) == 1:
            # First host in subcluster, need to create conductor group and so on
            subcid = self.args['subcid']
            hgroup = build_host_group(getattr(self.config, ROLE_HOST_TYPE[host_role]), hosts)
            hgroup.properties.conductor_group_id = subcid
            self._create_conductor_group(hgroup)
            self._create_host_secrets(hgroup)
            self._create_host_group(hgroup)
            self._issue_tls(hgroup)

        self.properties = getattr(self.config, ROLE_HOST_TYPE[host_role])
        self.properties.conductor_group_id = self.args['subcid']
