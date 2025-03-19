# -*- coding: utf-8 -*-
"""
DBaaS Internal API MongoDB charts
"""

from flask import g

from ...utils import config
from ...utils.register import DbaasOperation, Resource, register_request_handler
from .constants import MY_CLUSTER_TYPE
from .pillar import get_cluster_pillar


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.CHARTS)
def mongodb_cluster_charts(cluster, _):
    """
    Returns cluster charts
    """
    pillar = get_cluster_pillar(cluster)
    is_sharding_enabled = int(pillar.sharding_enabled)
    solomon_dashboards = ['internal-mdb-cluster-mongodb', 'internal-mdb-cluster-mongodb-sharded']
    yasm_dashboards = ['dbaas_mongodb_metrics', 'dbaas_mongodb_sharded_metrics']

    return [
        {
            'name': chart[0],
            'description': chart[1],
            'link': chart[2].format(
                cid=cluster['cid'],
                folderId=g.folder['folder_ext_id'],
                cluster_type=MY_CLUSTER_TYPE,
                console=config.get_console_address(),
                yasm_dashboard=yasm_dashboards[is_sharding_enabled],
                solomon_dashboard=solomon_dashboards[is_sharding_enabled],
            ),
        }
        for chart in config.cluster_charts(MY_CLUSTER_TYPE)
    ]
