# -*- coding: utf-8 -*-
"""
DBaaS Internal API Hadoop charts
"""

from ...utils.register import DbaasOperation, Resource, register_request_handler
from .constants import MY_CLUSTER_TYPE


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.CHARTS)
def hadoop_cluster_charts(*_):
    """
    Returns empty list for now
    """
    return []
