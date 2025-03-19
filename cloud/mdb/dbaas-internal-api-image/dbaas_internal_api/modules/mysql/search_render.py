# -*- coding: utf-8 -*-
"""
DBaaS Internal API MySQL search renders
"""

from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import ClusterInfo
from .constants import MY_CLUSTER_TYPE
from .pillar import MySQLPillar


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.SEARCH_ATTRIBUTES)
def mysql_cluster_search_attributes(cluster: ClusterInfo) -> dict:
    """
    Collect MySQL specific search attributes
    """
    pillar = MySQLPillar(cluster.pillar)
    return {
        'databases': [d['name'] for d in pillar.databases(cluster.cid)],
        'users': [u['name'] for u in pillar.users(cluster.cid)],
    }
