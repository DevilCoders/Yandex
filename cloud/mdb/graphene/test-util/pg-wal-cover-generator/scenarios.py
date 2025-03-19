"""
WAL coverage test data generator scenarios
"""
import os
from contextlib import closing
from io import StringIO

TARGET_DB_NAME = 'cover_test_db'


def execute(cluster, queries, database=TARGET_DB_NAME):
    """
    Query execution helper
    """
    with closing(cluster.get_conn(database)) as conn:
        conn.autocommit = True
        cursor = conn.cursor()
        for query in queries:
            cursor.execute(query)


def create_drop_database(cluster):
    """
    Generate XLOG_DBASE_CREATE and XLOG_DBASE_DROP info for dbase_redo rmgr
    """
    execute(cluster, ['CREATE DATABASE test_rm_db', 'DROP DATABASE test_rm_db'], database='postgres')


def create_drop_tablespace(cluster):
    """
    Generate XLOG_TBLSPC_CREATE and XLOG_TBLSPC_DROP info for tblspc_redo rmgr
    """
    os.makedirs('/tmp/pg-wal-cover-generator', exist_ok=True)
    execute(
        cluster,
        ['CREATE TABLESPACE tmp LOCATION \'/tmp/pg-wal-cover-generator\'', 'DROP TABLESPACE tmp'],
        database='postgres',
    )
    os.rmdir('/tmp/pg-wal-cover-generator')


def create_target_db_with_extension(cluster):
    """
    Create target db and add pageinspect
    """
    execute(cluster, [f'CREATE DATABASE {TARGET_DB_NAME}'], database='postgres')
    execute(cluster, ['CREATE EXTENSION pageinspect'])


def create_table_rollback(cluster):
    """
    Create table, fill with data, abort
    """
    execute(
        cluster,
        [
            'BEGIN',
            'CREATE TABLE delete_me (id int PRIMARY KEY)',
            'INSERT INTO delete_me (id) SELECT d.id FROM generate_series(1, 100000) AS d (id)',
            'ROLLBACK',
        ],
    )


def create_nextval_sequence(cluster):
    """
    Create sequence and get nextval from it to emit XLOG_SEQ_LOG info for seq rmgr
    """
    execute(cluster, ['CREATE SEQUENCE test_seq AS bigint START 1', 'SELECT nextval(\'test_seq\')', 'COMMIT'])


def concurrent_fk(cluster):
    """
    Generate CREATE_ID, ZERO_MEM_PAGE, and ZERO_OFF_PAGE info for MultiXact rmgr
    """
    execute(
        cluster,
        [
            'CREATE TABLE ref_table (id int PRIMARY KEY, data text)',
            'CREATE TABLE ref_use_table (id int PRIMARY KEY, ref_id int NOT NULL, data text, '
            'CONSTRAINT fk_ref_table_use_ref_table FOREIGN KEY (ref_id) REFERENCES ref_table (id))',
            'INSERT INTO ref_table (id) VALUES (1)',
            'INSERT INTO ref_use_table (id, ref_id) VALUES (1, 1)',
        ],
    )

    conn1 = cluster.get_conn(TARGET_DB_NAME)
    conn1.autocommit = True
    cursor1 = conn1.cursor()
    conn2 = cluster.get_conn(TARGET_DB_NAME)
    conn2.autocommit = True
    cursor2 = conn2.cursor()
    conn3 = cluster.get_conn(TARGET_DB_NAME)
    conn3.autocommit = True
    cursor3 = conn3.cursor()

    cursor1.execute('BEGIN')
    cursor2.execute('BEGIN')
    cursor3.execute('BEGIN')
    cursor1.execute('SELECT * FROM ref_table WHERE id = 1 FOR NO KEY UPDATE')
    cursor2.execute('UPDATE ref_use_table SET data = \'test\' WHERE ref_id = 1')
    cursor3.execute('SELECT * FROM ref_table WHERE id = 1 FOR KEY SHARE')
    cursor1.execute('UPDATE ref_table SET data = \'test\' WHERE id = 1')
    cursor1.execute('COMMIT')
    cursor2.execute('COMMIT')
    cursor3.execute('COMMIT')


def create_table(cluster):
    """
    Create table in target db with some fields and indexes
    """
    queries = [
        'BEGIN',
        'CREATE TABLE test(field1 int, field2 text, field3 int[], field4 point) WITH (fillfactor=10)',
        'CREATE INDEX idx_test_brin ON test USING brin (field1)',
        'CREATE INDEX idx_test_btree ON test USING btree (field2)',
        'CREATE INDEX idx_test_gin ON test USING gin (field3) WITH (gin_pending_list_limit = 256)',
        'CREATE INDEX idx_test_gist ON test USING gist (field4)',
        'CREATE INDEX idx_test_spgist ON test USING spgist (field2)',
        'CREATE INDEX idx_test_hash ON test USING hash (field1)',
        'COMMIT',
    ]
    execute(cluster, queries)


def truncate_table(cluster):
    """
    Generate LOCK and UPDATE info for heap rmgr
    """
    execute(cluster, ['TRUNCATE TABLE test'])


def insert_delete_vacuum(cluster):
    """
    Insert 10000 records into test table, delete them, and vacuum table
    """
    queries = ['BEGIN']
    for i in range(10000):
        queries.append(
            'INSERT INTO test(field1, field2, field3, field4) VALUES '
            f'({i}, \'test{i}\', \'{{{i}}}\', point({i}, {i*2}))'
        )
    queries.append('COMMIT')
    queries.append('DELETE FROM test')
    queries.append('SELECT brin_desummarize_range(\'idx_test_brin\', 0)')  # desummarization will be forced by vacuum
    queries.append('VACUUM')
    queries.append('BEGIN')
    for i in range(10000):
        queries.append(
            'INSERT INTO test(field1, field2, field3, field4) VALUES '
            f'({10000-i}, \'test{10000-i}\', \'{{{10000-i}}}\', point({10000-i}, {(10000-i)*2}))'
        )
    queries.append('COMMIT')
    execute(cluster, queries)


def multiinsert(cluster):
    """
    Generate MULTI_INSERT info for heap2 rmgr
    """
    with closing(cluster.get_conn(TARGET_DB_NAME)) as conn:
        conn.autocommit = True
        cursor = conn.cursor()
        data = StringIO('1\n2\n3\n4\n5\n')
        cursor.copy_from(data, 'test', columns=('field1',))


def prepared_xacts(cluster):
    """
    Generate PREPARE, COMMIT_PREPARED, and ABORT_PREPARED info for xact rmgr
    """
    execute(cluster, ['BEGIN', 'INSERT INTO test(field1) values (1)', 'PREPARE TRANSACTION \'rollback_me\''])
    execute(cluster, ['ROLLBACK PREPARED \'rollback_me\''])
    execute(cluster, ['BEGIN', 'INSERT INTO test(field1) values (2)', 'PREPARE TRANSACTION \'commit_me\''])
    execute(cluster, ['COMMIT PREPARED \'commit_me\''])


def savepoints(cluster):
    """
    Generate
    """
    queries = ['BEGIN']
    # We need number of subtrans to exceed PGPROC_MAX_CACHED_SUBXIDS (64) to emit XLOG_XACT_ASSIGNMENT info
    for i in range(65):
        queries.append(f'INSERT INTO test(field1) values ({i})')
        queries.append(f'SAVEPOINT test_savepoint{i}')
    queries.append('COMMIT')
    execute(cluster, queries)


def vacuum_freeze(cluster):
    """
    Generate FREEZE_PAGE info for heap2 rmgr
    """
    execute(cluster, ['VACUUM FREEZE test'])


def bloom(cluster):
    """
    Bloom index uses generic WAL
    """
    execute(cluster, ['CREATE EXTENSION bloom'])
    execute(cluster, ['CREATE INDEX idx_test_bloom ON test USING bloom (field1)'])


def clog_gen(cluster):
    """
    Generate 32768 transactions to force allocation of new clog page
    """
    queries = []
    for _ in range(32768):
        queries += ['BEGIN', 'INSERT INTO test(field1) VALUES (1)', 'ROLLBACK']

    execute(cluster, queries)
    execute(cluster, ['VACUUM FREEZE'])
    execute(cluster, ['VACUUM FREEZE'], database='postgres')


def alter_table(cluster):
    """
    Set fillfactor to 100 and sort table with btree index
    """
    execute(
        cluster,
        [
            'BEGIN',
            'ALTER TABLE test SET (fillfactor=100)',
            'CLUSTER test USING idx_test_btree',
            'COMMIT',
        ],
    )


def update(cluster):
    """
    Generate
    """
    execute(
        cluster,
        [
            'BEGIN',
            'UPDATE test SET field1 = field1 / 2 WHERE (field1 % 10) = 0',
            'UPDATE test SET field2 = field1::text || \'test\'',
            'COMMIT',
            'VACUUM',
        ],
    )


def switch_xlog(cluster):
    """
    Generate SWITCH info for xlog rmgr
    """
    execute(cluster, ['SELECT pg_switch_wal()'])


GEN_SCENARIOS = [
    create_drop_database,
    create_drop_tablespace,
    create_target_db_with_extension,
    create_table_rollback,
    create_nextval_sequence,
    concurrent_fk,
    create_table,
    truncate_table,
    insert_delete_vacuum,
    multiinsert,
    prepared_xacts,
    savepoints,
    vacuum_freeze,
    bloom,
    clog_gen,
    alter_table,
    update,
    switch_xlog,
]
