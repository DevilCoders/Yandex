"""Stands cluster maps (CLOUD-35467)"""


import sys
from typing import Dict

from bootstrap.common.rdbms.db import Db

from .stands import get_stand  # FIXME: avoid circular imports
from .types import StandClusterMapVersion, StandClusterMap
from .utils import decorate_exported


__all__ = [
    "get_stand_cluster_map",
    "get_stand_cluster_map_version",

    "update_stand_cluster_map",
]


def get_stand_cluster_map(stand_name: str, *, db: Db) -> StandClusterMap:
    stand = get_stand(stand_name, db=db)

    id, _, grains, cluster_configs_version, yc_ci_version, bootstrap_templates_version = db.select_one(
        "stand_cluster_maps", stand.id, column_name="stand_id",
    )

    result = StandClusterMap(id, stand.name, grains, cluster_configs_version, yc_ci_version, bootstrap_templates_version)

    return result


def get_stand_cluster_map_version(stand_name: str, *, db: Db) -> StandClusterMapVersion:
    stand = get_stand(stand_name, db=db)

    query = """
        SELECT stand_cluster_maps.cluster_configs_version, stand_cluster_maps.yc_ci_version,
               stand_cluster_maps.bootstrap_templates_version
        FROM stand_cluster_maps
        INNER JOIN stands ON
            stand_cluster_maps.stand_id = stands.id
        WHERE
            stands.name = %s;
    """

    return StandClusterMapVersion(*db.select_by_condition(query, (stand.name, ), ensure_one_result=True))


def update_stand_cluster_map(stand_name: str, data: Dict, *, db: Db) -> StandClusterMap:
    stand_cluster_map = get_stand_cluster_map(stand_name, db=db)

    for attr_name, attr_value in data.items():
        setattr(stand_cluster_map, attr_name, attr_value)
    db.update_one("stand_cluster_maps", list(data.keys()), stand_cluster_map)

    db.conn.commit()

    return stand_cluster_map


decorate_exported(sys.modules[__name__])
