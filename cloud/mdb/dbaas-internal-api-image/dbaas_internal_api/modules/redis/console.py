# -*- coding: utf-8 -*-
"""
Redis API specific to console.
"""

from ...utils.feature_flags import ensure_no_feature_flag
from ...utils.register import DbaasOperation, Resource as ResourceEnum, register_request_handler
from ...utils.version import fill_updatable_to
from .utils import fill_persistence_modes, fill_tls_supported
from .constants import MY_CLUSTER_TYPE, UPGRADE_PATHS


@register_request_handler(MY_CLUSTER_TYPE, ResourceEnum.CONSOLE_CLUSTERS_CONFIG, DbaasOperation.INFO)
def get_clusters_config_handler(res: dict) -> None:
    """
    Performs Redis specific logic for 'get_clusters_config'
    """
    ensure_no_feature_flag('MDB_REDIS_DISABLE_LEGACY_CONSOLE_API')
    fill_updatable_to(res['available_versions'], UPGRADE_PATHS)
    fill_tls_supported(res)
    fill_persistence_modes(res)
