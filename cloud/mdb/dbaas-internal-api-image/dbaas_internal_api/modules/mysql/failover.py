"""
DBaaS Internal API Mysql start failover
"""
from datetime import timedelta
from typing import Any, Dict, Optional  # noqa

from ...core.exceptions import HostNotExistsError, InvalidFailoverTarget, NotEnoughHAHosts
from ...core.types import Operation
from ...utils import metadb
from ...utils.host import get_hosts
from ...utils.metadata import StartClusterFailoverMetadata
from ...utils.operation_creator import create_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from .constants import MY_CLUSTER_TYPE
from .host_pillar import MysqlHostPillar
from .pillar import get_cluster_pillar
from .traits import MySQLOperations, MySQLTasks


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.START_FAILOVER)
def mysql_start_failover(cluster, host_name: Optional[str] = None, **_) -> Operation:
    """
    Start failover on MySQL cluster
    """
    pillar = get_cluster_pillar(cluster)
    task_args = {
        'zk_hosts': pillar.zk_hosts,
    }  # type: Dict[str, Any]

    hosts = get_hosts(cluster['cid'])
    if host_name:
        if host_name not in hosts:
            raise HostNotExistsError(host_name)
        host_pillar = MysqlHostPillar(metadb.get_fqdn_pillar(host_name))
        if host_pillar.repl_source:
            raise InvalidFailoverTarget(host_name)
        task_args['target_host'] = host_name
    else:
        # will check suitable hosts
        suitable_cnt = 0
        for host in hosts:
            host_pillar = MysqlHostPillar(metadb.get_fqdn_pillar(host))
            if not host_pillar.repl_source:
                suitable_cnt += 1

        if suitable_cnt < 2:
            raise NotEnoughHAHosts()

    return create_operation(
        task_type=MySQLTasks.start_failover,
        operation_type=MySQLOperations.start_failover,
        metadata=StartClusterFailoverMetadata(),
        time_limit=timedelta(hours=1),
        cid=cluster['cid'],
        task_args=task_args,
    )
