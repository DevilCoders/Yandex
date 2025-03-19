"""
Test for NodePool
"""

from collections import namedtuple
from unittest.mock import MagicMock, Mock

import pytest
from psycopg2 import extensions as _ext

from dbaas_internal_api.dbs.postgresql.node_pool import (
    NodePool,
    PoolExhaustedError,
    PoolStats,
    UnexpectedConnectionError,
)
from dbaas_internal_api.dbs.postgresql.node_pool import __name__ as MODULE
from dbaas_internal_api.dbs.postgresql.node_pool import refresh_connection

# pylint: disable=invalid-name, missing-docstring


class TestNodePool:
    Mocks = namedtuple('Mocks', ['connect', 'refresh_connection', 'lock', 'cursor'])

    def mock_it(self, mocker):
        cursor = MagicMock()
        cursor.__enter__.return_value = MagicMock()

        conn = MagicMock(closed=False)
        conn.return_value.cursor.return_value = cursor

        connect = mocker.patch(
            MODULE + '.psycopg2.connect',
            autospec=True,
        )
        connect.return_value = conn

        refresh = mocker.patch(
            MODULE + '.refresh_connection',
            autospec=True,
        )
        lock = mocker.patch(
            MODULE + '.threading.Lock',
            autospec=True,
        )
        return self.Mocks(connect=connect, refresh_connection=refresh, lock=lock, cursor=cursor)

    def test_init(self, mocker):
        self.mock_it(mocker)

        pool = NodePool(
            5,
            10,
            host='test.local',
            port=6432,
        )

        assert pool.stats == PoolStats(used=0, free=10, open=5)

    def test_getconn_update_stats(self, mocker):
        self.mock_it(mocker)

        pool = NodePool(
            5,
            10,
            host='test.local',
            port=6432,
        )

        pool.getconn()
        assert pool.stats == PoolStats(used=1, free=9, open=5)

    def test_putconn_update_stats(self, mocker):
        self.mock_it(mocker)

        pool = NodePool(
            5,
            10,
            host='test.local',
            port=6432,
        )

        pool.putconn(pool.getconn())
        assert pool.stats == PoolStats(used=0, free=10, open=5)

    def test_getconn_return_connections_in_cycle(self, mocker):
        mk = self.mock_it(mocker)
        conns = []
        for _ in range(10):
            conns.append(MagicMock())
            conns[-1].return_value.cursor.return_value = mk.cursor
        mk.connect.side_effect = conns

        pool = NodePool(
            3,
            10,
            host='test.local',
            port=6432,
        )

        assert pool.getconn() == conns[0]
        pool.putconn(conns[0])
        assert pool.getconn() == conns[1]
        pool.putconn(conns[1])
        assert pool.getconn() == conns[2]
        pool.putconn(conns[2])
        assert pool.getconn() == conns[0]

    def test_close_overflow_connections(self, mocker):
        mk = self.mock_it(mocker)

        pool = NodePool(
            3,
            10,
            host='test.local',
            port=6432,
        )

        for _ in range(3):
            pool.getconn()
        pool.putconn(pool.getconn())

        mk.connect.return_value.close.assert_called_once_with()

    def test_put_close_broken_connections(self, mocker):
        mk = self.mock_it(mocker)
        mk.refresh_connection.return_value = False

        pool = NodePool(
            3,
            10,
            host='test.local',
            port=6432,
        )

        conn = pool.getconn()
        pool.putconn(conn)

        mk.connect.return_value.close.assert_called_once_with()

    def test_put_close_connections_if_we_ask_it_to_close(self, mocker):
        mk = self.mock_it(mocker)
        mk.refresh_connection.return_value = True

        pool = NodePool(
            3,
            10,
            host='test.local',
            port=6432,
        )

        conn = pool.getconn()
        pool.putconn(conn, close=True)

        mk.connect.return_value.close.assert_called_once_with()

    def test_put_raise_exception_when_get_unexpected_connection(self, mocker):
        self.mock_it(mocker)
        strange_connection = Mock(closed=False)
        strange_connection.get_dsn_parameters.return_value = {
            'host': 'another-host',
            'dbname': 'ydb',
        }

        pool = NodePool(
            3,
            10,
            host='test.local',
            port=6432,
        )
        with pytest.raises(UnexpectedConnectionError):
            pool.putconn(strange_connection)

    def test_raise_PoolError_if_more_then_maxconns_used(self, mocker):
        self.mock_it(mocker)
        pool = NodePool(
            3,
            10,
            host='test.local',
            port=6432,
        )
        for _ in range(10):
            pool.getconn()
        with pytest.raises(PoolExhaustedError):
            pool.getconn()

    def test_closeall_close_all_connections(self, mocker):
        mk = self.mock_it(mocker)

        pool = NodePool(
            3,
            10,
            host='test.local',
            port=6432,
        )
        for _ in range(4):
            pool.getconn()
        pool.closeall()

        assert mk.connect.return_value.close.call_count == 4

    def test_recreate_with_used_connections(self, mocker):
        mk = self.mock_it(mocker)

        pool = NodePool(
            3,
            10,
            host='test.local',
            port=6432,
        )
        used_conn = pool.getconn()
        pool.recreate()
        assert mk.connect.return_value.close.call_count == 2
        assert pool.stats == PoolStats(used=1, free=9, open=3)
        pool.putconn(used_conn)
        assert pool.stats == PoolStats(used=0, free=10, open=3)

    def test_recreate_with_overflow(self, mocker):
        mk = self.mock_it(mocker)

        pool = NodePool(
            3,
            10,
            host='test.local',
            port=6432,
        )
        for _ in range(5):
            _ = pool.getconn()
        pool.recreate()
        assert mk.connect.return_value.close.call_count == 0
        assert pool.stats == PoolStats(used=5, free=5, open=5)

    def test_c_getconn(self, mocker):
        self.mock_it(mocker)

        pool = NodePool(
            5,
            10,
            host='test.local',
            port=6432,
        )

        with pool.c_getconn():
            assert pool.stats == PoolStats(used=1, free=9, open=5)
        assert pool.stats == PoolStats(used=0, free=10, open=5)

    def test_grow(self, mocker):
        self.mock_it(mocker)

        pool = NodePool(
            5,
            10,
            host='test.local',
            port=6432,
        )
        pool.grow()
        assert pool.stats == PoolStats(used=0, free=10, open=5)
        pool.minconn = 7
        pool.grow()
        assert pool.stats == PoolStats(used=0, free=10, open=7)


class Test_refresh_connection:
    def mk(self, closed, status=None):
        conn = Mock(closed=closed)
        conn.get_transaction_status.return_value = status
        return conn

    def test_closed_connection_cant_be_reused(self):
        closed_conn = self.mk(closed=True)
        assert not refresh_connection(closed_conn)

    def test_open_conn_in_unknown_tx_state_cant_be_reused(self):
        broken_conn = self.mk(closed=False, status=_ext.TRANSACTION_STATUS_UNKNOWN)
        assert not refresh_connection(broken_conn)

    def test_idle_conn_can_be_reused_and_rollback_not_called(self):
        idle_conn = self.mk(closed=False, status=_ext.TRANSACTION_STATUS_IDLE)
        assert refresh_connection(idle_conn)

    def test_conn_in_tx_rollbacked_and_can_be_resused(self):
        conn_in_tx = self.mk(closed=False, status=_ext.TRANSACTION_STATUS_ACTIVE)
        assert refresh_connection(conn_in_tx)
        conn_in_tx.rollback.assert_called_once_with()

    def test_active_conn_cancelled_and_rollbacked_and_can_be_resused(self):
        active_conn = self.mk(closed=False, status=_ext.TRANSACTION_STATUS_ACTIVE)
        assert refresh_connection(active_conn)
        active_conn.cancel.assert_called_once_with()
        active_conn.rollback.assert_called_once_with()
