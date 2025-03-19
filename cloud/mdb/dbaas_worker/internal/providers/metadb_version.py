"""
MetaDB versions manipulation module
"""

from dataclasses import dataclass
from dbaas_common import retry, tracing
from typing import Optional

from ..metadb import DatabaseConnectionError
from ..query import execute
from .base_metadb import BaseMetaDBProvider


@dataclass
class DbaasVersion:
    cid: str
    subcid: Optional[str]
    shard_id: Optional[str]
    component: str
    major_version: str  # e.g '8.0'
    minor_version: str  # e.g '8.0.17'
    package_version: str  # e.g. '8.0.17-8-1.bionic+yandex1'
    edition: str
    pinned: bool


class MetadbVersions(BaseMetaDBProvider):
    """
    DBaaS MetaDB Versions provider
    """

    @retry.on_exception(DatabaseConnectionError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('MetaDB Select')
    def get_cluster_version(self, cluster_id: str, component: str) -> Optional[DbaasVersion]:
        """
        Get mysql version from dbaas.versions
        """
        with self.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                res = execute(
                    cur,
                    'get_version',
                    cid=cluster_id,
                    component=component,
                )
                if res:
                    return DbaasVersion(
                        cid=res[0]['cid'],
                        subcid=res[0]['subcid'],
                        shard_id=res[0]['shard_id'],
                        component=res[0]['component'],
                        major_version=res[0]['major_version'],
                        minor_version=res[0]['minor_version'],
                        package_version=res[0]['package_version'],
                        edition=res[0]['edition'],
                        pinned=res[0]['pinned'],
                    )

                return None
