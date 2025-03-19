"""
DBaaS Internal API Redis charts
"""

from flask import g

from ...utils import config
from ...utils.feature_flags import ensure_no_feature_flag
from ...utils.register import DbaasOperation, Resource, register_request_handler
from .constants import MY_CLUSTER_TYPE


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.CHARTS)
def redis_cluster_charts(cluster, _):
    """
    Returns cluster charts
    """
    ensure_no_feature_flag('MDB_REDIS_DISABLE_LEGACY_CONSOLE_API')
    return [
        {
            'name': chart[0],
            'description': chart[1],
            'link': chart[2].format(
                cid=cluster['cid'],
                folderId=g.folder['folder_ext_id'],
                cluster_type=MY_CLUSTER_TYPE,
                console=config.get_console_address(),
            ),
        }
        for chart in config.cluster_charts(MY_CLUSTER_TYPE)
    ]
