"""
DBaaS Internal API PostgreSQL start failover
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
from .pillar import get_cluster_pillar
from .traits import PostgresqlOperations, PostgresqlTasks


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.START_FAILOVER)
def postgresql_start_failover(cluster, host_name: Optional[str] = None, **_) -> Operation:
    """
    Start failover on PostgreSQL cluster
    """
    ha_hosts = metadb.get_pg_ha_hosts_by_cid(cluster['cid'])
    if len(ha_hosts) < 2:
        raise NotEnoughHAHosts()

    pillar = get_cluster_pillar(cluster)
    task_args = {
        'zk_hosts': pillar.pgsync.get_zk_hosts(),
    }  # type: Dict[str, Any]
    if host_name:
        if host_name not in get_hosts(cluster['cid']):
            raise HostNotExistsError(host_name)
        if host_name not in ha_hosts:
            raise InvalidFailoverTarget(host_name)
        task_args['target_host'] = host_name

    return create_operation(
        task_type=PostgresqlTasks.start_failover,
        operation_type=PostgresqlOperations.start_failover,
        metadata=StartClusterFailoverMetadata(),
        time_limit=timedelta(hours=1),
        cid=cluster['cid'],
        task_args=task_args,
    )
