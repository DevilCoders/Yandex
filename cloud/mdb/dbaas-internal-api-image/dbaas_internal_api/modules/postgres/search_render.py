# -*- coding: utf-8 -*-
"""
DBaaS Internal API PostgreSQL search renders
"""

from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import ClusterInfo
from .constants import MY_CLUSTER_TYPE
from .pillar import PostgresqlClusterPillar


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.SEARCH_ATTRIBUTES)
def postgresql_cluster_search_attributes(cluster: ClusterInfo) -> dict:
    """
    Collect postgresql specific search attributes
    """
    pillar = PostgresqlClusterPillar(cluster.pillar)
    return {
        'databases': pillar.databases.get_names(),
        'users': [u.name for u in pillar.pgusers.get_public_users()],
    }
