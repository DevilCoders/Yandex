import os

from hamcrest import assert_that, equal_to, has_item, has_length
import psycopg2

from cloud.mdb.recipes.postgresql.lib import env_postgres_addr
from cloud.mdb.pg.pgproxy.s3db.recipe.lib import CLUSTER_CONFIG


def _parts_count(subcluster_config):
    return len(subcluster_config)


def _hosts_count(subcluster_config):
    count = 0
    for shard in subcluster_config:
        count += len(shard)
    return count


def _make_connstring(host, port, dbname):
    return f'host={host} port={port} dbname={dbname}'


def _get_control_records():
    part_id = 0
    for shard in CLUSTER_CONFIG['s3meta']:
        part_id += 1
        for host in shard:
            addr_env, port_env = env_postgres_addr(host['name'])
            addr = os.environ.get(addr_env)
            port = os.environ.get(port_env)
            yield ('meta', part_id, host['name'], _make_connstring(addr, port, 's3meta'))
    for shard in CLUSTER_CONFIG['s3db']:
        part_id += 1
        for host in shard:
            addr_env, port_env = env_postgres_addr(host['name'])
            addr = os.environ.get(addr_env)
            port = os.environ.get(port_env)
            yield ('db', part_id, host['name'], _make_connstring(addr, port, 's3db'))


class TestConnection:
    def test_pgmeta_tables_filled(self):
        pgmeta_name = CLUSTER_CONFIG['pgmeta'][0][0]['name']
        host_env, port_env = env_postgres_addr(pgmeta_name)
        host = os.environ.get(host_env)
        port = os.environ.get(port_env)
        dsn = f'host={host} port={port} dbname=s3db'
        with psycopg2.connect(dsn) as conn:
            with conn.cursor() as cursor:
                # Test clusters
                cursor.execute('SELECT * FROM clusters')
                rows = cursor.fetchall()
                assert_that(rows, has_length(2))

                # Test parts
                cursor.execute('SELECT * FROM parts')
                rows = cursor.fetchall()
                assert_that(
                    rows, has_length(_parts_count(CLUSTER_CONFIG['s3meta']) + _parts_count(CLUSTER_CONFIG['s3db']))
                )

                hosts_count = _hosts_count(CLUSTER_CONFIG['s3meta']) + _hosts_count(CLUSTER_CONFIG['s3db'])
                # Test hosts
                cursor.execute('SELECT * FROM hosts')
                rows = cursor.fetchall()
                assert_that(rows, has_length(hosts_count))

                # Test connections
                cursor.execute('SELECT * FROM connections')
                rows = cursor.fetchall()
                assert_that(rows, has_length(hosts_count))

                # Test priorities
                cursor.execute('SELECT * FROM priorities')
                rows = cursor.fetchall()
                assert_that(rows, has_length(hosts_count))

                # Test all together
                cursor.execute(
                    """
                    SELECT c.name,
                        part_id,
                        h.host_name,
                        cc.conn_string
                    FROM priorities p
                        JOIN parts pp USING (part_id)
                        JOIN clusters c USING (cluster_id)
                        JOIN hosts h USING (host_id)
                        JOIN connections cc USING (conn_id)
                    ORDER BY part_id, host_id
                """
                )
                rows = cursor.fetchall()
                for record in _get_control_records():
                    assert_that(rows, has_item(record))

    def test_pgproxy_tables_filled(self):
        pgproxy_name = CLUSTER_CONFIG['pgproxy'][0][0]['name']
        host_env, port_env = env_postgres_addr(pgproxy_name)
        host = os.environ.get(host_env)
        port = os.environ.get(port_env)
        dsn = f'host={host} port={port} dbname=s3db'
        with psycopg2.connect(dsn) as conn:
            with conn.cursor() as cursor:
                # Test clusters
                cursor.execute('SELECT * FROM plproxy.clusters')
                rows = cursor.fetchall()
                assert_that(rows, has_length(2))

                # Test parts
                cursor.execute('SELECT * FROM plproxy.parts')
                rows = cursor.fetchall()
                assert_that(
                    rows, has_length(_parts_count(CLUSTER_CONFIG['s3meta']) + _parts_count(CLUSTER_CONFIG['s3db']))
                )

                hosts_count = _hosts_count(CLUSTER_CONFIG['s3meta']) + _hosts_count(CLUSTER_CONFIG['s3db'])
                # Test hosts
                cursor.execute('SELECT * FROM plproxy.hosts')
                rows = cursor.fetchall()
                assert_that(rows, has_length(hosts_count))

                # Test connections
                cursor.execute('SELECT * FROM plproxy.connections')
                rows = cursor.fetchall()
                assert_that(rows, has_length(hosts_count))

                # Test priorities
                cursor.execute('SELECT * FROM plproxy.priorities')
                rows = cursor.fetchall()
                assert_that(rows, has_length(hosts_count))

                # Test all together
                cursor.execute(
                    """
                    SELECT c.name,
                        part_id,
                        h.host_name,
                        cc.conn_string
                    FROM plproxy.priorities p
                        JOIN plproxy.parts pp USING (part_id)
                        JOIN plproxy.clusters c USING (cluster_id)
                        JOIN plproxy.hosts h USING (host_id)
                        JOIN plproxy.connections cc USING (conn_id)
                    ORDER BY part_id, host_id
                """
                )
                rows = cursor.fetchall()
                for record in _get_control_records():
                    assert_that(rows, has_item(record))

                cursor.execute('SELECT * FROM plproxy.dynamic_query_db(\'SELECT 1\') AS (col int)')
                (result,) = cursor.fetchone()
                assert_that(result, equal_to(1))
                cursor.execute('SELECT * FROM plproxy.dynamic_query_meta(\'SELECT 1\') AS (col int)')
                (result,) = cursor.fetchone()
                assert_that(result, equal_to(1))

                # Test plproxy.get_connstring
                i = 0
                for shard in ['01', '02']:
                    addr_env, port_env = env_postgres_addr(f's3meta{shard}')
                    addr = os.environ.get(addr_env)
                    port = os.environ.get(port_env)
                    cursor.execute(
                        'SELECT * FROM plproxy.get_connstring(%(cluster)s, %(shard)s)',
                        {'cluster': 'meta_rw', 'shard': i},
                    )
                    (result,) = cursor.fetchone()
                    assert_that(result, equal_to(_make_connstring(addr, port, 's3meta')))
                    i += 1
                i = 0
                for shard in ['01', '02']:
                    addr_env, port_env = env_postgres_addr(f's3db{shard}')
                    addr = os.environ.get(addr_env)
                    port = os.environ.get(port_env)
                    cursor.execute(
                        'SELECT * FROM plproxy.get_connstring(%(cluster)s, %(shard)s)', {'cluster': 'db_rw', 'shard': i}
                    )
                    (result,) = cursor.fetchone()
                    assert_that(result, equal_to(_make_connstring(addr, port, 's3db')))
                    i += 1
