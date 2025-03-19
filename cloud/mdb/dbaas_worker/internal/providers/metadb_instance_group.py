"""
MetaDB host manipulation module
"""

from dbaas_common import retry

from ..metadb import DatabaseConnectionError
from ..query import execute
from .base_metadb import BaseMetaDBProvider
from .common import Change


class MetadbInstanceGroup(BaseMetaDBProvider):
    """
    DBaaS MetaDB instance group provider
    """

    # pylint: disable=no-member
    @retry.on_exception(DatabaseConnectionError, factor=10, max_wait=60, max_tries=6)
    def update(self, subcid, instance_group_id):
        """
        Set field value for host
        """
        with self.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                rev = self.lock_cluster(conn, self.task['cid'])
                execute(
                    cur,
                    'update_instance_group',
                    fetch=False,
                    **{
                        'cid': self.task['cid'],
                        'subcid': subcid,
                        'instance_group_id': instance_group_id,
                        'rev': rev,
                    },
                )
                self.complete_cluster_change(conn, self.task['cid'], rev)
                self.add_change(Change(f'metadb_instance_group.{subcid}.id', 'updated', rollback=Change.noop_rollback))

    @retry.on_exception(DatabaseConnectionError, factor=10, max_wait=60, max_tries=6)
    def get(self, subcid):
        """
        Gets instance group id by subcid from metadb
        """
        with self.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                rows = execute(
                    cur,
                    'get_instance_group',
                    subcid=subcid,
                )
                if rows:
                    return rows[0]['instance_group_id']

    @retry.on_exception(DatabaseConnectionError, factor=10, max_wait=60, max_tries=6)
    def get_subcid_by_instance_group_id(self, instance_group_id):
        """
        Gets subcluster id by instance group from metadb
        """
        with self.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                rows = execute(
                    cur,
                    'get_subcluster_by_instance_group_id',
                    instance_group_id=instance_group_id,
                )
                if rows:
                    return rows[0]['subcid']
