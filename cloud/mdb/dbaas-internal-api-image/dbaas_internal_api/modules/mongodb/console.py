# -*- coding: utf-8 -*-
"""
MongoDB API specific to console.
"""

from ...utils.register import DbaasOperation
from ...utils.register import Resource as ResourceEnum
from ...utils.register import register_request_handler
from ...utils.version import fill_updatable_to
from .constants import MY_CLUSTER_TYPE, UPGRADE_PATHS
from .utils import update_fcvs


@register_request_handler(MY_CLUSTER_TYPE, ResourceEnum.CONSOLE_CLUSTERS_CONFIG, DbaasOperation.INFO)
def get_clusters_config_handler(res: dict) -> None:
    """
    Performs MongoDB specific logic for 'get_clusters_config'
    """
    fill_updatable_to(res['available_versions'], UPGRADE_PATHS)
    update_fcvs(res)
