# -*- coding: utf-8 -*-
"""
ClickHouse API specific to console.
"""
from ...utils.feature_flags import ensure_no_feature_flag
from ...utils.register import DbaasOperation
from ...utils.register import Resource as ResourceEnum
from ...utils.register import register_request_handler
from ...utils.version import iter_valid_versions
from .constants import MY_CLUSTER_TYPE
from .traits import ClickhouseClusterTraits


@register_request_handler(MY_CLUSTER_TYPE, ResourceEnum.CONSOLE_CLUSTERS_CONFIG, DbaasOperation.INFO)
def get_clusters_config_handler(res: dict) -> None:
    """
    Performs ClickHouse specific logic for 'get_clusters_config'
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_CONSOLE_API')

    res['ml_model_name'] = ClickhouseClusterTraits.ml_model_name
    res['ml_model_uri'] = ClickhouseClusterTraits.ml_model_uri
    res['format_schema_name'] = ClickhouseClusterTraits.format_schema_name
    res['format_schema_uri'] = ClickhouseClusterTraits.format_schema_uri
    fill_updatable_to(MY_CLUSTER_TYPE, res['available_versions'])


def fill_updatable_to(cluster_type: str, versions: list):
    """
    Fill 'updatable_to' list in every version entry.
    List consists of all versions that bigger than original and all versions that less, until first
    "'downgradable': False" entry in config
    """
    all_versions = sorted(list(iter_valid_versions(cluster_type)), key=lambda x: x[0], reverse=True)

    def _get_valid_updates(cur_version):
        updatable_versions = []
        last_downgradable_version = None
        for version, obj in all_versions:
            if version > cur_version:
                if not obj.get('deprecated'):
                    updatable_versions.append(version)
                continue

            if last_downgradable_version:
                return updatable_versions

            if obj.get('downgradable') is False:
                last_downgradable_version = version

            if version != cur_version and not obj.get('deprecated') is True:
                updatable_versions.append(version)

        return updatable_versions

    for cur_version in versions:
        cur_version['updatable_to'] = _get_valid_updates(cur_version['id'])
