"""
MetaDB manipulation module
"""

from contextlib import closing, contextmanager

from dbaas_common import tracing

from ..metadb import get_master_conn
from ..query import execute
from .common import BaseProvider


class BaseMetaDBProvider(BaseProvider):
    """
    Base DBaaS MetaDB
    """

    @tracing.trace('MetaDB Lock Cluster')
    def lock_cluster(self, conn, cid, x_request_id=None):  # pylint: disable=no-self-use
        """
        Lock cluster. Return rev
        """
        tracing.set_tag('cluster.id', cid)
        if x_request_id:
            tracing.set_tag('request_id', x_request_id)

        cur = conn.cursor()
        rows = execute(
            cur,
            'lock_cluster',
            cid=cid,
            x_request_id=x_request_id,
        )
        if not rows:
            raise RuntimeError(f'Got no rows while try lock cluster {cid}')
        return rows[0]['rev']

    @tracing.trace('MetaDB Complete Cluster Change')
    def complete_cluster_change(self, conn, cid, rev):  # pylint: disable=no-self-use
        """
        Call complete_cluster_change
        """
        tracing.set_tag('cluster.id', cid)
        tracing.set_tag('cluster.revision', rev)

        cur = conn.cursor()
        execute(cur, 'complete_cluster_change', cid=cid, rev=rev)

    @contextmanager
    def get_master_conn(self):
        """
        Get connection to master
        """
        master_conn = get_master_conn(self.config.main.metadb_dsn, self.config.main.metadb_hosts, self.logger)
        with closing(master_conn) as conn:
            yield conn
