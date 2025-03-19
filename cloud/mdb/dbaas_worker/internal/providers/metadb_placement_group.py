"""
MetaDB placement group manipulation module
"""

from dbaas_common import retry, tracing

from ..metadb import DatabaseConnectionError
from ..query import execute
from .base_metadb import BaseMetaDBProvider
from .common import Change


class MetadbPlacementGroup(BaseMetaDBProvider):
    """
    DBaaS MetaDB placement group provider
    """

    # pylint: disable=no-member
    @retry.on_exception(DatabaseConnectionError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('MetaDB Update')
    def update(self, placement_group_id, status, fqdn):
        """
        Set placement_group_id and update status for placement group
        """
        cid = self.task['cid']
        tracing.set_tag('cluster.id', cid)
        tracing.set_tag('cluster.placement_group_id', placement_group_id)

        with self.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                rev = self.lock_cluster(conn, cid)
                execute(
                    cur,
                    'update_placement_group',
                    fetch=False,
                    **{
                        'placement_group_id': placement_group_id,
                        'status': status,
                        'cid': cid,
                        'rev': rev,
                        'fqdn': fqdn,
                    },
                )
                self.complete_cluster_change(conn, cid, rev)
                self.add_change(
                    Change(
                        f'metadb_placement_group.{cid}.{placement_group_id}',
                        'updated',
                        rollback=Change.noop_rollback,
                    )
                )
