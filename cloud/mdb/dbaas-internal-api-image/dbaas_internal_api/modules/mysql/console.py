# -*- coding: utf-8 -*-
"""
MySQL handlers for console API.
"""

from ...utils.register import DbaasOperation
from ...utils.register import Resource as ResourceEnum
from ...utils.register import register_request_handler
from ...utils.version import fill_updatable_to
from .constants import MY_CLUSTER_TYPE
from .validation import UPGRADE_PATHS


@register_request_handler(MY_CLUSTER_TYPE, ResourceEnum.CONSOLE_CLUSTERS_CONFIG, DbaasOperation.INFO)
def get_clusters_config_handler(res: dict) -> None:
    """
    Performs MySQL specific logic for 'get_clusters_config'
    """

    #
    # THIS METHOD IS DEPRECATED
    # Add new features to go-api
    #
    fill_updatable_to(res['available_versions'], UPGRADE_PATHS)
