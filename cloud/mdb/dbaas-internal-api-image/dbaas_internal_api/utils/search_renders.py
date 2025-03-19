"""
Search renders
"""
from datetime import datetime, timezone
from typing import Any, Dict, List, Optional

from . import metadb
from ..core.exceptions import UnsupportedHandlerError
from .register import DbaasOperation, Resource, get_cluster_traits, get_request_handler
from .types import ClusterInfo


def _render_timestamp(timestamp: datetime) -> str:
    return timestamp.astimezone(timezone.utc).isoformat()


def _get_service_slug(cluster_type: str) -> str:
    return get_cluster_traits(cluster_type).service_slug


def _base_cluster(
    cluster: ClusterInfo,
    timestamp: datetime,
    folder_ext_id: str,
    cloud_ext_id: str,
    reindex_timestamp: Optional[datetime],
) -> dict:
    doc = {
        'timestamp': _render_timestamp(timestamp),
        'resource_type': 'cluster',
        'resource_id': cluster.cid,
        'service': _get_service_slug(cluster.type),
        'permission': 'mdb.all.read',
        'folder_id': folder_ext_id,
        'cloud_id': cloud_ext_id,
        'name': cluster.name,
        # resource_path holds info about where to check our permission
        # in our case it's a folder
        'resource_path': [
            {
                'resource_id': folder_ext_id,
                'resource_type': 'resource-manager.folder',
            }
        ],
    }
    if reindex_timestamp is not None:
        doc['reindex_timestamp'] = _render_timestamp(reindex_timestamp)
    return doc


def render_deleted_cluster(
    cluster: ClusterInfo,
    timestamp: datetime,
    deleted_at: datetime,
    folder_ext_id: str,
    cloud_ext_id: str,
    reindex_timestamp: Optional[datetime] = None,
) -> dict:
    """
    Render deleted cluster
    """
    doc = _base_cluster(cluster, timestamp, folder_ext_id, cloud_ext_id, reindex_timestamp)
    doc['deleted'] = _render_timestamp(deleted_at)

    return doc


def _render_cluster(
    cluster: ClusterInfo,
    timestamp: datetime,
    custom_attributes: dict,
    folder_ext_id: str,
    cloud_ext_id: str,
    reindex_timestamp: Optional[datetime],
) -> dict:
    """
    Render cluster doc
    """
    doc = _base_cluster(cluster, timestamp, folder_ext_id, cloud_ext_id, reindex_timestamp)
    doc['attributes'] = {
        'name': cluster.name,
        'description': cluster.description or '',
        'labels': cluster.labels or {},
        **custom_attributes,
    }
    return doc


def _get_cluster_hosts(cid: str) -> List[str]:
    fqdns = [h['fqdn'] for h in metadb.get_hosts(cid)]
    fqdns.sort()
    return fqdns


def make_doc(
    cluster: ClusterInfo, timestamp: datetime, folder_ext_id: str, cloud_ext_id: str, reindex_timestamp: datetime = None
) -> dict:
    """
    Return search document for a cluster
    """
    if cluster.status.is_deleting():
        return render_deleted_cluster(cluster, timestamp, timestamp, folder_ext_id, cloud_ext_id, reindex_timestamp)

    attributes: Dict[str, Any] = {'hosts': _get_cluster_hosts(cluster.cid)}

    try:
        attributes_handler = get_request_handler(cluster.type, Resource.CLUSTER, DbaasOperation.SEARCH_ATTRIBUTES)
        attributes.update(attributes_handler(cluster))
    except UnsupportedHandlerError:
        pass

    return _render_cluster(cluster, timestamp, attributes, folder_ext_id, cloud_ext_id, reindex_timestamp)
