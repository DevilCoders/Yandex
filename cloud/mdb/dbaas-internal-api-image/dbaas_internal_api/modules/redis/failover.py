"""
DBaaS Internal API Redis start failover
"""
from datetime import timedelta

from ...core.exceptions import PreconditionFailedError
from ...core.types import Operation
from ...utils.feature_flags import ensure_no_feature_flag
from ...utils.host import get_hosts
from ...utils.metadata import StartClusterFailoverMetadata
from ...utils.operation_creator import create_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from .constants import MY_CLUSTER_TYPE
from .hosts import get_masters
from .pillar import RedisPillar
from .traits import RedisOperations, RedisTasks


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.START_FAILOVER)
def redis_start_failover(cluster, host_names=None, **_) -> Operation:
    """
    Start failover on Redis cluster
    """
    ensure_no_feature_flag('MDB_REDIS_DISABLE_LEGACY_CLUSTER_OPERATE_API')
    cid = cluster['cid']
    cluster_pillar = RedisPillar(cluster['value'])
    cluster_hosts = get_hosts(cid)
    if host_names is None:
        host_names = []
    if len(cluster_hosts) == 1:
        raise PreconditionFailedError('Operation is not permitted on a single node cluster.')

    if len(host_names) > 1:
        raise PreconditionFailedError('Failover on redis cluster does not support multiple hostnames at this time.')

    if not set(host_names).issubset(cluster_hosts):
        raise PreconditionFailedError('Hostnames you specified are not the part of cluster.')

    if cluster_pillar.is_cluster_enabled() and len(host_names) < 1:
        raise PreconditionFailedError(
            'To perform failover on sharded redis cluster at least one host must be specified.'
        )

    if not cluster_pillar.is_cluster_enabled() and len(host_names) < 1:
        masters = get_masters(cluster)
        if masters is None or len(masters) == 0:
            raise PreconditionFailedError(
                'Unable to find masters for cluster. Failover is not safe. Aborting failover.'
            )
        host_names = masters

    return create_operation(
        task_type=RedisTasks.start_failover,
        operation_type=RedisOperations.start_failover,
        metadata=StartClusterFailoverMetadata(),
        time_limit=timedelta(hours=1),
        cid=cid,
        task_args={"failover_hosts": host_names},
    )
