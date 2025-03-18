import ydb
from concurrent.futures import thread

import threading
import cityhash


YT_MAX_SIZE = 16777216
YDB_MAX_SIZE = 4194304


PUT_ITEM = '''
    PRAGMA TablePathPrefix("{}");

    DECLARE $partition AS Uint64;
    DECLARE $hash AS String;
    DECLARE $type AS Uint64;
    DECLARE $data0 AS String;
    DECLARE $data1 AS String;
    DECLARE $data2 AS String;
    DECLARE $data3 AS String;

    REPLACE INTO objects
        (partition, hash, type, data0, data1, data2, data3)
    VALUES
        ($partition, $hash, $type, $data0, $data1, $data2, $data3)
'''


GET_ITEM = '''
    PRAGMA TablePathPrefix("{}");

    DECLARE $partition AS Uint64;
    DECLARE $hash AS String;

    SELECT
        hash, type, data0, data1, data2, data3
    FROM objects
    WHERE
        partition = $partition
        AND hash = $hash
'''


PUT_SVN_HEAD = '''
    PRAGMA TablePathPrefix("{}");

    DECLARE $svn_head AS String;

    REPLACE INTO kv
        (key, value)
    VALUES
        ("svn_head", $svn_head)
'''


GET_SVN_HEAD = '''
    PRAGMA TablePathPrefix("{}");

    SELECT
        key, value
    FROM
        kv
    WHERE
        key = "svn_head"
'''


class YdbClient(object):

    def __init__(self, endpoint, database, token, threads=8):
        self._endpoint = endpoint
        self._database = database

        driver_config = ydb.DriverConfig(endpoint, database=database, auth_token=token)

        self._lock = threading.Lock()
        self._driver = ydb.Driver(driver_config)
        self._driver.wait(timeout=5)
        self._session_pool = ydb.SessionPool(self._driver, size=threads+10)

        self._tl = threading.local()
        self._pool = thread.ThreadPoolExecutor(max_workers=threads)

        self._put_item_query = PUT_ITEM.format(self._database)
        self._get_item_query = GET_ITEM.format(self._database)
        self._put_svn_head_query = PUT_SVN_HEAD.format(self._database)
        self._get_svn_head_query = GET_SVN_HEAD.format(self._database)

    def put_item(self, h, t, d):
        if len(d) > YT_MAX_SIZE:
            raise RuntimeError('Too big data: {}'.format(len(d)))

        def ydb_operation(session):
            prepared_query = session.prepare(self._put_item_query)
            session.transaction(ydb.SerializableReadWrite()).execute(
                prepared_query,
                {
                    '$partition': cityhash.hash64(h),
                    '$hash': h,
                    '$type': t,
                    '$data0': d[0:YDB_MAX_SIZE],
                    '$data1': d[YDB_MAX_SIZE:2*YDB_MAX_SIZE],
                    '$data2': d[2*YDB_MAX_SIZE:3*YDB_MAX_SIZE],
                    '$data3': d[3*YDB_MAX_SIZE:4*YDB_MAX_SIZE],
                },
                commit_tx=True
            )
        self._session_pool.retry_operation_sync(ydb_operation)

    def put_many(self, items):

        def func(chunk):
            # TODO(kikht): I don't think that merging multiple item into single
            # transaction will help much because we still execute queries
            # synchrounously and require db round-trip for each query.
            # We should either merge multiple requests in one query
            # or issue them all asynchrounously.
            # I think first approach is better.
            def ydb_operation(session):
                prepared_query = session.prepare(self._put_item_query)
                txn = session.transaction(ydb.SerializableReadWrite())
                for i, (h, t, d) in enumerate(chunk):
                    txn.execute(
                        prepared_query,
                        {
                            '$partition': cityhash.hash64(h),
                            '$hash': h,
                            '$type': t,
                            '$data0': d[0:YDB_MAX_SIZE],
                            '$data1': d[YDB_MAX_SIZE:2*YDB_MAX_SIZE],
                            '$data2': d[2*YDB_MAX_SIZE:3*YDB_MAX_SIZE],
                            '$data3': d[3*YDB_MAX_SIZE:4*YDB_MAX_SIZE],
                        },
                        commit_tx=(i + 1 == len(chunk))
                    )
            self._session_pool.retry_operation_sync(ydb_operation)

        c = []
        size = 0
        fs = []
        for (h, t, d) in items:
            c.append((h, t, d))
            size += len(d)

            if len(c) > 1000 or size > 2**20:
                f = self._pool.submit(func, c)
                c = []
                size = 0
                fs.append(f)

        if c:
            f = self._pool.submit(func, c)
            c = []
            size = 0
            fs.append(f)

        def ret():
            for f in fs:
                f.result()

        return ret

    def get_item(self, h):
        def ydb_operation(session):
            prepared_query = session.prepare(self._get_item_query)
            res = session.transaction(ydb.SerializableReadWrite()).execute(
                prepared_query,
                {
                    '$partition': cityhash.hash64(h),
                    '$hash': h,
                },
                commit_tx=True
            )
            return res[0].rows

        rows = self._session_pool.retry_operation_sync(ydb_operation)

        assert len(rows) <= 1
        if len(rows) == 0:
            raise KeyError(h)

        row = rows[0]
        return row['type'], row['data0'] + row['data1'] + row['data2'] + row['data3']

    def put_svn_head(self, svn_head):
        def ydb_operation(session):
            prepared_query = session.prepare(self._put_svn_head_query)
            session.transaction(ydb.SerializableReadWrite()).execute(
                prepared_query,
                {
                    '$svn_head': str(svn_head),
                },
                commit_tx=True
            )
        self._session_pool.retry_operation_sync(ydb_operation)

    def get_svn_head(self):
        def ydb_operation(session):
            prepared_query = session.prepare(self._get_svn_head_query)
            res = session.transaction(ydb.SerializableReadWrite()).execute(
                prepared_query,
                {
                },
                commit_tx=True
            )
            return res[0].rows

        rows = self._session_pool.retry_operation_sync(ydb_operation)
        assert len(rows) == 1
        row = rows[0]
        assert row['key'] == 'svn_head'

        return row['value']
