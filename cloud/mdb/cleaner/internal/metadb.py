import logging
from typing import List

import psycopg2
import psycopg2.extras

from .models import Cluster

GET_VISIBLE_CLUSTERS_TO_DELETE_SQL = """
SELECT
    c.cid,
    c.type,
    c.name,
    c.created_at
FROM dbaas.clusters c
JOIN dbaas.folders f ON c.folder_id = f.folder_id
WHERE (%(folder_id)s IS NULL OR f.folder_ext_id = %(folder_id)s)
  AND (%(label_key)s IS NULL OR c.cid NOT IN
       (SELECT cid FROM dbaas.cluster_labels
        WHERE label_key = %(label_key)s AND label_value = %(label_value)s))
  AND now() - c.created_at > %(max_age)s * interval '1 second'
  AND code.visible(c)
"""

GET_RUNNING_CLUSTERS_TO_STOP_SQL = """
SELECT
    c.cid,
    c.type,
    c.name,
    c.created_at
FROM dbaas.clusters c
JOIN dbaas.folders f ON c.folder_id = f.folder_id
WHERE (%(folder_id)s IS NULL OR f.folder_ext_id = %(folder_id)s)
  AND (%(label_key)s IS NULL OR c.cid NOT IN
       (SELECT cid FROM dbaas.cluster_labels
        WHERE label_key = %(label_key)s AND label_value = %(label_value)s))
  AND now() - c.created_at > %(max_age)s * interval '1 second'
  AND c.status = 'RUNNING'::dbaas.cluster_status
"""

GET_STOPPED_CLUSTERS_TO_DELETE_SQL = """
SELECT
    c.cid,
    c.type,
    c.name,
    c.created_at
FROM dbaas.clusters c
JOIN dbaas.folders f ON c.folder_id = f.folder_id
JOIN dbaas.worker_queue q ON q.cid = c.cid
WHERE (%(folder_id)s IS NULL OR f.folder_ext_id = %(folder_id)s)
  AND (%(label_key)s IS NULL OR c.cid NOT IN
       (SELECT cid FROM dbaas.cluster_labels
        WHERE label_key = %(label_key)s AND label_value = %(label_value)s))
  AND c.status = 'STOPPED'::dbaas.cluster_status
GROUP BY c.cid, c.type, c.name, c.created_at
HAVING now() - max(q.end_ts) > %(max_age)s * interval '1 second';
"""


class MetaDB:
    """
    MetaDB adapter.
    """

    def __init__(self, config):
        self.config = config
        self.connection = None

    def __enter__(self):
        self.connection = psycopg2.connect(connection_factory=LoggingConnection, **self.config['metadb'])
        self.connection.initialize(logging.getLogger())
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.connection is not None:
            self.connection.close()
            self.connection = None

    def get_visible_clusters_to_delete(self) -> List[Cluster]:
        """
        Get clusters in simple delete strategy.
        """
        assert self.connection

        with self._cursor() as cursor:
            cursor.execute(
                GET_VISIBLE_CLUSTERS_TO_DELETE_SQL,
                {
                    'folder_id': self.config.get('folder'),
                    'max_age': self.config['max_age_sec'],
                    'label_key': self.config.get('label_key'),
                    'label_value': self.config.get('label_value'),
                },
            )
            return cursor.fetchall()

    def get_running_clusters_to_stop(self) -> List[Cluster]:
        """
        Get clusters to stop in stop-delete strategy.
        """
        assert self.connection

        with self._cursor() as cursor:
            cursor.execute(
                GET_RUNNING_CLUSTERS_TO_STOP_SQL,
                {
                    'folder_id': self.config.get('folder'),
                    'max_age': self.config['max_age_sec'],
                    'label_key': self.config.get('label_key'),
                    'label_value': self.config.get('label_value'),
                },
            )
            return cursor.fetchall()

    def get_stopped_clusters_to_delete(self) -> List[Cluster]:
        """
        Get clusters to delete in stop-delete strategy.
        """
        assert self.connection

        with self._cursor() as cursor:
            cursor.execute(
                GET_STOPPED_CLUSTERS_TO_DELETE_SQL,
                {
                    'folder_id': self.config.get('folder'),
                    'max_age': self.config['max_age_sec'],
                    'label_key': self.config.get('label_key'),
                    'label_value': self.config.get('label_value'),
                },
            )
            return cursor.fetchall()

    def is_read_only(self) -> bool:
        """
        Check whether DB is in read-only mode or not.
        """
        assert self.connection

        with self._cursor() as cursor:
            cursor.execute('show transaction_read_only')
            return cursor.fetchone().transaction_read_only == 'on'

    def _cursor(self):
        return self.connection.cursor(cursor_factory=psycopg2.extras.NamedTupleCursor)


class LoggingConnection(psycopg2.extras.LoggingConnection):
    """
    Subclass of psycopg2.extras.LoggingConnection that decodes binary
    representation of SQL queries to strings.
    """

    def filter(self, msg, curs):
        """
        Decode log message passed to logger as "bytes".
        """
        if isinstance(msg, bytes):
            msg = msg.decode()
        return msg
