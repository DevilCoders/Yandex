# encoding: utf-8

from random import choice
from .database import Database
from psycopg2 import OperationalError
from psycopg2.extras import NamedTupleCursor
from s3meta import S3MetaDB
from s3db import S3DB
from exceptions import ConnstringException


DC_PRIORITY = ['MAN', 'IVA', 'VLA', 'SAS', 'MYT']


def _get_db_class(cluster):
    if cluster == 'meta':
        return S3MetaDB
    elif cluster == 'db':
        return S3DB
    else:
        return Database


class PgmetaDB(Database):

    def get_connections(self, cluster=None, shard=None):
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            SELECT * FROM get_connections()
              WHERE (%(cluster)s IS NULL OR name = %(cluster)s)
                AND (%(shard)s IS NULL OR shard_id = %(shard)s)
        """, {'cluster': cluster, 'shard': shard})
        self.log_last_query(cur)
        return cur.fetchall()

    def get_master(self, cluster, shard, **connstring_kwargs):
        cur = self.create_cursor()
        cur.execute("""
              SELECT conn_string FROM get_connections()
                WHERE name = %(cluster)s AND shard_id = %(shard)s
        """, {'cluster': cluster, 'shard': shard})
        self.log_last_query(cur)
        db_class = _get_db_class(cluster)
        return db_class(
            PgmetaDB.build_connstring([row[0] for row in cur.fetchall()], target_session_attrs='read-write'),
            shard_id=shard,
            **connstring_kwargs
        )

    def get_replica(self, cluster, shard, max_lag=30, **connstring_kwargs):
        # Returns open replica with lag < max_lag or master
        cur = self.create_cursor()
        cur.execute("""
            SELECT conn_string FROM get_connections()
                WHERE name = %(cluster)s AND shard_id = %(shard)s
        """, {'cluster': cluster, 'shard': shard})
        self.log_last_query(cur)
        hosts = []
        db_class = _get_db_class(cluster)
        for connstring in cur.fetchall():
            try:
                db = db_class(connstring[0] + ' connect_timeout=1', shard_id=shard, **connstring_kwargs)
                is_closed, is_replica, lag = db.get_state()
                if not is_closed and not (is_replica and lag > max_lag):
                    hosts.append((db, is_replica, lag))
            except OperationalError:
                pass

        if len(hosts) > 1:
            hosts = [host for host in hosts if host[1]]     # only replicas
        return choice(hosts)[0] if len(hosts) > 0 else None

    def get_s3meta_list(self, **connstring_kwargs):
        cur = self.create_cursor()
        cur.execute("""
            SELECT shard_id, string_agg(conn_string, ',') FROM get_connections()
              WHERE name='meta'
              GROUP BY shard_id ORDER BY shard_id;
        """)
        self.log_last_query(cur)
        user = connstring_kwargs.get('user')
        return [S3MetaDB(
            PgmetaDB.build_connstring(connstrings.split(','), **connstring_kwargs),
            shard_id=shard_id,
            user=user
        ) for shard_id, connstrings in cur.fetchall()]

    def get_s3meta_rw_by_bucket(self, bid, **connstring_kwargs):
        for s3meta in self.get_s3meta_list(target_session_attrs='read-write', **connstring_kwargs):
            bucket = s3meta.get_bucket(bid)
            if bucket:
                return s3meta, bucket
        return None, None

    def get_bucket_meta_shard_id(self, bucket_name):
        cur = self.create_cursor()
        cur.execute("""
            SELECT (hashtext(%(bucket_name)s) & (
                    SELECT count(distinct shard_id) - 1 FROM get_connections() where name='meta'
                ))::int
        """, {'bucket_name': bucket_name})
        self.log_last_query(cur)
        return cur.fetchone()[0]

    def get_shards_count(self, cluster):
        cur = self.create_cursor()
        cur.execute("""
            SELECT count(distinct shard_id) FROM get_connections() WHERE name = %(cluster)s
        """, {'cluster': cluster})
        self.log_last_query(cur)
        return cur.fetchone()[0]

    @staticmethod
    def build_connstring(connstrings, **connstring_kwargs):
        result_host = []
        result_kwargs = {
            'target_session_attrs': 'any',
            'connect_timeout': 1,
            'sslmode': 'verify-full',
            'sslrootcert': '/opt/yandex/allCAs.pem',
        }

        for connstring in connstrings:
            for attr in connstring.split():
                key, value = attr.split('=')
                if key == 'host':
                    result_host.append(value)
                else:
                    if key in result_kwargs and result_kwargs[key] != value:
                        raise ConnstringException('Wrong attr %s in connstring %s' % (key, connstring))
                    result_kwargs[key] = value
        result_kwargs['host'] = ','.join(result_host)
        for key, value in connstring_kwargs.iteritems():
            if value is not None:
                result_kwargs[key] = value
        return ' '.join(['%s=%s' % (k, v) for k, v in result_kwargs.items()])

    def replica_has_less_priority(self, hostname, cluster):
        cur = self.create_cursor()
        cur.execute("""
            SELECT dc, conn_string
            FROM get_connections()
            WHERE (name, shard_id) IN (
                SELECT name, shard_id
                FROM get_connections()
                WHERE conn_string ~ %(hostname)s
                  AND name=%(cluster)s
            )
        """, {'cluster': cluster, 'hostname': hostname})
        self.log_last_query(cur)
        # sort by index in DC_PRIORITY, get first row -> conn_string
        less_priority_connstring = sorted(cur.fetchall(), key=lambda row: DC_PRIORITY.index(row[0]))[0][1]
        return hostname in less_priority_connstring

    def get_closed_hosts_in_cluster(self, hostname, **connstring_kwargs):
        cur = self.create_cursor()
        cur.execute("""
            SELECT conn_string
            FROM get_connections()
            WHERE (name, shard_id) IN (
                SELECT name, shard_id
                FROM get_connections()
                WHERE conn_string ~ %(hostname)s
            )
        """, {'hostname': hostname})
        self.log_last_query(cur)
        closed_hosts = []
        for connstring in cur.fetchall():
            try:
                if not Database(connstring[0] + ' connect_timeout=1', **connstring_kwargs).check_is_open():
                    closed_hosts.append(connstring[0])
            except OperationalError:
                closed_hosts.append(connstring[0])
        return closed_hosts

    def get_shard_id(self, hostname):
        cur = self.create_cursor()
        cur.execute("""
            SELECT shard_id
            FROM get_connections()
            WHERE conn_string ~ %(hostname)s
            LIMIT 1
        """, {'hostname': hostname})
        self.log_last_query(cur)
        row = cur.fetchone()
        return row[0] if row else None
