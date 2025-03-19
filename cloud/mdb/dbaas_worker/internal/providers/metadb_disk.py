"""
MetaDB disk manipulation module
"""

from dbaas_common import retry, tracing

from ..metadb import DatabaseConnectionError
from ..query import execute
from .base_metadb import BaseMetaDBProvider
from .common import Change


class MetadbDisks(BaseMetaDBProvider):
    """
    DBaaS MetaDB disk provider
    """

    # pylint: disable=no-member
    @retry.on_exception(DatabaseConnectionError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('MetaDB Update')
    def update(self, fqdn, local_id, mount_point, status, disk_id, host_disk_id):
        """
        Set field value for host
        """
        tracing.set_tag('cluster.id', self.task['cid'])
        tracing.set_tag('cluster.host.fqdn', fqdn)
        tracing.set_tag('cluster.host.disk', disk_id)

        with self.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                rev = self.lock_cluster(conn, self.task['cid'])
                execute(
                    cur,
                    'update_disk',
                    fetch=False,
                    **{
                        'fqdn': fqdn,
                        'local_id': local_id,
                        'mount_point': mount_point,
                        'status': status,
                        'disk_id': disk_id,
                        'host_disk_id': host_disk_id,
                        'cid': self.task['cid'],
                        'rev': rev,
                    },
                )
                self.complete_cluster_change(conn, self.task['cid'], rev)
                self.add_change(Change(f'metadb_disk.{fqdn}.{disk_id}', 'updated', rollback=Change.noop_rollback))

    def get_disks_info(self, fqdn):
        """
        Get disks info (local_id, mount_point)
        """
        with self.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                rows = execute(
                    cur,
                    'get_disks_info',
                    fetch=True,
                    **{
                        'fqdn': fqdn,
                    },
                )
                if rows:
                    return rows[0]['local_id'], rows[0]['mount_point']
                return None, None
