"""
Billing estimator helpers
"""

from typing import List, Union

from .types import ExistedHostResources, RequestedHostResources
from .validation import get_flavor_by_name


def generate_metric(
    host_spec: dict,
    resources: Union[RequestedHostResources, ExistedHostResources],
    cluster_type: str,
    roles: List[str],
    folder_id: str,
    schema: str = 'mdb.db.generic.v1',
    on_dedicated_host: bool = False,
) -> dict:
    """
    Generate billing metric (without usage part)
    """
    flavor = get_flavor_by_name(resources.resource_preset_id)

    return {
        'tags': {
            'public_ip': 1 if host_spec.get('assign_public_ip', False) else 0,
            'disk_type_id': resources.disk_type_id,
            'cluster_type': cluster_type,
            'disk_size': resources.disk_size,
            'resource_preset_id': resources.resource_preset_id,
            'platform_id': flavor.get('platform_id', ''),
            'cores': int(flavor.get('cpu_limit', 0)),
            'core_fraction': flavor.get('cpu_fraction', 100),
            'memory': flavor.get('memory_limit', 0),
            'software_accelerated_network_cores': flavor.get('io_cores_limit', 0),
            'roles': roles,
            'online': 1,
            'on_dedicated_host': 1 if on_dedicated_host else 0,
        },
        'folder_id': folder_id,
        'schema': schema,
    }
