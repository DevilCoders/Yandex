# -*- coding: utf-8 -*-
"""
DBaaS Internal API e2e functions
"""

from flask import g
from .config import get_e2e_config


def is_cluster_for_e2e(cluster_name: str) -> bool:
    e2e_config = get_e2e_config()
    e2e_folder = e2e_config.get("folder_id")
    e2e_cluster = e2e_config.get("cluster_name")
    if e2e_folder is None or e2e_cluster is None:
        return False

    return e2e_folder == g.folder["folder_ext_id"] and cluster_name == e2e_cluster
