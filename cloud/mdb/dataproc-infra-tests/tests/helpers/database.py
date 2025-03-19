"""
Database helpers
"""

import json
import os

import psycopg2
import redis


def pg_connect(context, conf: dict = {}):
    """
    Create connection to specified pg service
    """
    conf = context.conf['projects']['metadb']
    dsn = (
        f"host={conf['host']} port={conf['port']} dbname={conf['dbname']} "
        f"user={conf['user']} password={conf['password']} sslmode=require"
    )
    return psycopg2.connect(dsn)


def pg_dump_csv(context, path):
    """
    Dump all user tables in the format of csv files
    """
    with pg_connect(context) as conn:
        cur = conn.cursor()
        cur.execute(
            """
            SELECT schemaname,tablename
            FROM pg_tables
            WHERE schemaname
            NOT IN ('pg_catalog', 'information_schema', 'public');
            """
        )
        schemas_tables = cur.fetchall()

        for schema, table in schemas_tables:
            schema_dir = os.path.join(path, schema)
            os.makedirs(schema_dir, exist_ok=True)

            sql = 'COPY {0}.{1} TO STDOUT WITH CSV HEADER'.format(schema, table)
            with open(os.path.join(schema_dir, table + '.csv'), 'w') as file:
                cur.copy_expert(sql, file)


def redis_dump_json(context, path):
    conf = context.conf['projects']['redis']
    result = {}
    client = redis.Redis(host=conf['host'], password=conf['password'])
    for key in client.keys():
        key_type = client.type(key)
        if key_type == b'hash':
            val = {}
            redis_hash = client.hgetall(key)
            for hash_key, hash_val in redis_hash.items():
                val[hash_key.decode()] = json.loads(hash_val)
            result[key.decode()] = val
        elif key_type == b'string':
            key_value = client.get(key)
            try:
                result[key.decode()] = json.loads(client.get(key))
            except json.decoder.JSONDecodeError as exception:
                raise ValueError(
                    f'{exception.msg} \n Can not parse JSON value from redis. Key: {key} Value: {key_value}'
                )
        elif key_type == b'none':
            continue
        else:
            raise ValueError(f'Unknown key type: {key_type}')

    os.makedirs(path, exist_ok=True)
    path = os.path.join(path, 'dump.json')
    with open(path, 'w') as file_handler:
        json.dump(result, file_handler, sort_keys=True, indent=4)
