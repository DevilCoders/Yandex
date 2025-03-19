"""
MetaDB disk placement group manipulation module
"""

from dbaas_common import retry, tracing

from ..metadb import DatabaseConnectionError
from ..query import execute
from .base_metadb import BaseMetaDBProvider
from .common import Change


class MetadbDiskPlacementGroup(BaseMetaDBProvider):
    """
    DBaaS MetaDB disk placement group provider
    """

    # pylint: disable=no-member
    @retry.on_exception(DatabaseConnectionError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('MetaDB Update')
    def update(self, disk_placement_group_id, status, local_id):
        """
        Set disk_placement_group_id and update status for disk placement group
        """
        cid = self.task['cid']
        tracing.set_tag('cluster.id', cid)
        tracing.set_tag('cluster.disk_placement_group', disk_placement_group_id)

        with self.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                rev = self.lock_cluster(conn, cid)
                execute(
                    cur,
                    'update_disk_placement_group',
                    fetch=False,
                    **{
                        'disk_placement_group_id': disk_placement_group_id,
                        'status': status,
                        'local_id': local_id,
                        'cid': cid,
                        'rev': rev,
                    },
                )
                self.complete_cluster_change(conn, cid, rev)
                self.add_change(
                    Change(
                        f'metadb_disk_placement_group.{cid}.{disk_placement_group_id}',
                        'updated',
                        rollback=Change.noop_rollback,
                    )
                )
