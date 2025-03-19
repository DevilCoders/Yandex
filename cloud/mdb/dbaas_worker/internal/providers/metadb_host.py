"""
MetaDB host manipulation module
"""

from dbaas_common import retry, tracing

from ..metadb import DatabaseConnectionError
from ..query import execute
from .base_metadb import BaseMetaDBProvider
from .common import Change


class MetadbHost(BaseMetaDBProvider):
    """
    DBaaS MetaDB host provider
    """

    # pylint: disable=no-member
    @retry.on_exception(DatabaseConnectionError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('MetaDB Update')
    def update(self, fqdn, field, value):
        """
        Set field value for host
        """
        tracing.set_tag('cluster.id', self.task['cid'])
        tracing.set_tag('cluster.host.fqdn', fqdn)

        with self.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                rev = self.lock_cluster(conn, self.task['cid'])
                execute(
                    cur,
                    f'update_host_{field}',
                    fetch=False,
                    **{
                        'fqdn': fqdn,
                        field: value,
                        'cid': self.task['cid'],
                        'rev': rev,
                    },
                )
                self.complete_cluster_change(conn, self.task['cid'], rev)
                self.add_change(Change(f'metadb_host.{fqdn}.{field}', 'updated', rollback=Change.noop_rollback))

    @retry.on_exception(DatabaseConnectionError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('MetaDB Host Info')
    def get_host_info(self, fqdn):
        """
        Get host info (cluster_id, cluster_type, instance_id, local_id)
        """
        with self.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                res = execute(cur, 'get_host_info', fqdn=fqdn)
                if res:
                    return res[0]

    @retry.on_exception(DatabaseConnectionError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('MetaDB Host Info by vtype id')
    def get_host_info_by_vtype_d(self, vtype_id):
        """
        Get host info (cluster_id, cluster_type, instance_id, local_id)
        """
        with self.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                res = execute(cur, 'get_host_by_vtype_id', vtype_id=vtype_id)
                if res:
                    return res[0]['fqdn']
