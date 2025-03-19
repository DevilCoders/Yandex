"""
DBaaS Internal API cluster getters
"""

from datetime import datetime
from typing import List, Optional

from dbaas_common.dict import combine_dict

from ...core.exceptions import ClusterNotFound, ClusterTypeMismatchError
from .. import metadb
from ..logs import get_logger
from ..types import ClusterInfo, ClusterStatus, ClusterVisibility, ComparableEnum


def assert_cluster_type_matched(cluster: dict, cluster_type: str) -> None:
    """
    Raise error if cluster_type not matched
    """
    if cluster_type != cluster['type']:
        raise ClusterTypeMismatchError(cluster['cid'])


def get_cluster(cluster_id: str, cluster_type: str) -> dict:
    """
    Get cluster and check its type.
    """
    cluster = metadb.get_cluster(
        cid=cluster_id,
    )
    if cluster is None:
        raise ClusterNotFound(cluster_id)

    assert_cluster_type_matched(cluster, cluster_type)
    return cluster


def lock_cluster(cluster_id: str, cluster_type: str) -> dict:
    """
    Lock cluster and check its type.
    """
    cluster = metadb.lock_cluster(cid=cluster_id)
    if cluster is None:
        raise ClusterNotFound(cluster_id)

    assert_cluster_type_matched(cluster, cluster_type)
    return cluster


def get_subclusters(cluster: dict, combine_pillar=False) -> List[dict]:
    """
    Get subclusters with pillars for the specified cluster.

    If combine_pillar is True, pillar data of each subcluster is merged with
    cluster pillar.
    """
    subclusters = metadb.get_subcluster_pillars_by_cluster(cluster['cid'])

    if combine_pillar:
        for subcluster in subclusters:
            subcluster['value'] = combine_dict(cluster['value'], subcluster['value'])

    return subclusters


def get_subcluster(*, role: ComparableEnum, cluster_id: str = None, subclusters: List[dict] = None) -> dict:
    """
    Get subcluster with the specified role.

    Either cluster or subclusters must be passed in.
    """
    if subclusters is None:
        assert cluster_id
        subclusters = metadb.get_subcluster_pillars_by_cluster(cluster_id)

    for subcluster in subclusters:
        if role in subcluster['roles']:
            return subcluster

    raise RuntimeError('{0} subcluster not found'.format(role))


def get_cluster_info(cluster_id: str, cluster_type: str) -> ClusterInfo:
    """
    cast get_cluster ret into ClusterInfo
    """
    return ClusterInfo.make(get_cluster(cluster_id=cluster_id, cluster_type=cluster_type))


def locked_cluster_info(cluster_id: str, cluster_type: str) -> ClusterInfo:
    """
    Lock cluster and get it as ClusterInfo
    """
    return ClusterInfo.make(lock_cluster(cluster_id=cluster_id, cluster_type=cluster_type))


def get_cluster_info_assert_exists(
    cluster_id: str, cluster_type: str = None, include_deleted: bool = False
) -> ClusterInfo:
    """
    Return cluster info and fail with RuntimeError if cluster doesn't exist

    Should used in context where we know that cluster exists
    """
    cluster = metadb.get_cluster(
        cid=cluster_id,
        visibility=ClusterVisibility.visible_or_deleted if include_deleted else ClusterVisibility.visible,
    )
    if cluster is None:
        raise RuntimeError(f'Cluster with {cluster_id} not found')

    if cluster_type is not None:
        assert_cluster_type_matched(cluster, cluster_type)

    return ClusterInfo.make(cluster)


def get_cluster_info_at_time(cluster_id: str, timestamp: datetime) -> ClusterInfo:
    """
    Get cluster at specific time

    - Find rev that match that time
    - Get cluster data at that rev
    """
    rev = metadb.get_cluster_rev_by_timestamp(cid=cluster_id, timestamp=timestamp)
    cluster_dict = metadb.get_cluster_at_rev(cid=cluster_id, rev=rev)
    cluster = ClusterInfo.make(cluster_dict)
    get_logger().info("cluster at time %r is %r", timestamp, cluster)
    return cluster


def prepare_cluster_essence(cluster):
    """
    Prepare cluster type from query
    """
    cluster['status'] = ClusterStatus(cluster['status'])
    # TODO: make typo?
    return cluster


def get_all_clusters_essence_in_folder(
    cluster_type: Optional[str],
    limit: Optional[int],
    cluster_name: str = None,
    cid: str = None,
    env: str = None,
    page_token_name: str = None,
) -> List[dict]:
    """
    Return list of all cluster essence with specific type in folder
    """
    query_res = metadb.get_clusters_essence_by_folder(
        cluster_type=cluster_type,
        limit=limit,
        cluster_name=cluster_name,
        cid=cid,
        env=env,
        page_token_name=page_token_name,
    )
    return [prepare_cluster_essence(i) for i in query_res]


def get_all_clusters_info_in_folder(
    cluster_type: Optional[str],
    limit: Optional[int],
    cluster_name: str = None,
    cid: str = None,
    env: str = None,
    page_token_name: str = None,
    include_deleted: bool = False,
) -> List[ClusterInfo]:
    """
    Return list of all cluster with specific type in folder
    """
    ret = []  # List[ClusterInfo]
    for cluster_dict in metadb.get_clusters_by_folder(
        cluster_type=cluster_type,
        limit=limit,
        cluster_name=cluster_name,
        cid=cid,
        env=env,
        page_token_name=page_token_name,
        visibility=ClusterVisibility.visible_or_deleted if include_deleted else ClusterVisibility.visible,
    ):
        ret.append(ClusterInfo.make(cluster_dict))
    return ret


def get_shards(cluster: dict, role: ComparableEnum = None) -> List[dict]:
    """
    Return list of shard objects conforming to ShardSchema.
    """
    return [
        {
            'name': shard['name'],
            'shard_id': shard['shard_id'],
            'cluster_id': cluster['cid'],
        }
        for shard in metadb.get_shards(cid=cluster['cid'], role=role)
    ]


def find_shard_by_name(shards: List[dict], name: str, ignore_case: bool = False) -> Optional[dict]:
    """
    Find shard in list of shard db objects by name.
    """
    if ignore_case:
        gen = (shard for shard in shards if not name or shard['name'].lower() == name.lower())
    else:
        gen = (shard for shard in shards if not name or shard['name'] == name)

    return next(gen, None)
