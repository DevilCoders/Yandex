from typing import Optional

from dbaas_common import retry

from ..metadb import DatabaseConnectionError
from ..query import execute
from .base_metadb import BaseMetaDBProvider
from .common import Change


class SubclusterNodeGroup:
    subcluster_id: str
    kubernetes_cluster_id: str
    node_group_id: str

    def __init__(self, subcluster_id: str, kubernetes_cluster_id: str, node_group_id: str):
        self.subcluster_id = subcluster_id
        self.kubernetes_cluster_id = kubernetes_cluster_id
        self.node_group_id = node_group_id


class MetadbKubernetes(BaseMetaDBProvider):

    # pylint: disable=no-member
    @retry.on_exception(DatabaseConnectionError, factor=10, max_wait=60, max_tries=6)
    def update(self, subcid, kubernetes_cluster_id=None, node_group_id=None):
        with self.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                rev = self.lock_cluster(conn, self.task['cid'])
                execute(
                    cur,
                    'update_kubernetes_node_group',
                    fetch=False,
                    **{
                        'cid': self.task['cid'],
                        'subcid': subcid,
                        'kubernetes_cluster_id': kubernetes_cluster_id,
                        'node_group_id': node_group_id,
                        'rev': rev,
                    },
                )
                self.complete_cluster_change(conn, self.task['cid'], rev)
                self.add_change(
                    Change(
                        f'metadb_kubernetes_node_group.{subcid}.id',
                        'updated',
                        rollback=Change.noop_rollback,
                    )
                )

    @retry.on_exception(DatabaseConnectionError, factor=10, max_wait=60, max_tries=6)
    def get_info_by_subcid(self, subcid: str) -> Optional[SubclusterNodeGroup]:
        with self.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                rows = execute(
                    cur,
                    'get_kubernetes_node_group',
                    subcid=subcid,
                )
                if rows:
                    row = rows[0]
                    return SubclusterNodeGroup(subcid, row['kubernetes_cluster_id'], row['node_group_id'])
        return None

    @retry.on_exception(DatabaseConnectionError, factor=10, max_wait=60, max_tries=6)
    def get_info_by_node_group_id(self, node_group_id: str) -> Optional[SubclusterNodeGroup]:
        with self.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                rows = execute(
                    cur,
                    'get_subcluster_by_kubernetes_node_group_id',
                    node_group_id=node_group_id,
                )
                if rows:
                    row = rows[0]
                    return SubclusterNodeGroup(row['subcid'], row['kubernetes_cluster_id'], node_group_id)
        return None
