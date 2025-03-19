"""
Connection pooling to Postgres node

based on psycopg2.pool
"""

from typing import Optional, Generator
import threading
from collections import deque
from contextlib import contextmanager, suppress
from typing import NamedTuple

import psycopg2
from psycopg2 import extensions as _ext

# used only for annotations
from psycopg2.extras import RealDictConnection as Connection


class PoolError(Exception):
    """
    Base pool error
    """


class PoolExhaustedError(PoolError):
    """
    Connection pool exhausted
    """


class UnexpectedConnectionError(PoolError):
    """
    Unexpected connection
    """


def refresh_connection(conn: Connection) -> bool:
    """
    Refresh connection return True if it success
    """
    if conn.closed:
        return False

    status = conn.get_transaction_status()
    if status == _ext.TRANSACTION_STATUS_UNKNOWN:
        # server connection lost
        return False

    if status == _ext.TRANSACTION_STATUS_ACTIVE:
        # connection has running query
        ret = True
        try:
            conn.cancel()
            conn.rollback()
        except psycopg2.Error:
            ret = False
        return ret

    if status != _ext.TRANSACTION_STATUS_IDLE:
        # connection in error or in transaction
        ret = True
        try:
            conn.rollback()
        except psycopg2.Error:
            ret = False
        return ret

    # regular idle connection
    return True


def close_silently(conn: Connection) -> None:
    """
    Close connection eat exceptions
    """
    if not conn.closed:
        with suppress(Exception):
            conn.close()


PoolStats = NamedTuple('PoolStats', [('used', int), ('free', int), ('open', int)])


class NodePool:
    """
    PostgreSQL connection pool to one node
    """

    def __init__(self, minconn: int, maxconn: int, **connect_kwargs) -> None:
        """
        Initialize the connection pool
        """
        self.minconn = minconn
        self.maxconn = maxconn

        self._connect_kwargs = connect_kwargs

        self._all_connections: list[Connection] = []
        self._free_connections: deque[Connection] = deque()

        self._lock = threading.Lock()

        for _ in range(self.minconn):
            self._connect()

    @property
    def stats(self) -> PoolStats:
        """
        Return connections stats
        """
        used = len(self._all_connections) - len(self._free_connections)
        return PoolStats(
            used=used,
            free=self.maxconn - used,
            open=len(self._all_connections),
        )

    def __repr__(self) -> str:
        return f'NodePool(host={self.host}, stats={self.stats})'

    @property
    def host(self) -> Optional[str]:
        return self._connect_kwargs.get('host')

    def _connect(self) -> None:
        """
        Create a new connection and remember it
        """
        conn = psycopg2.connect(**self._connect_kwargs)

        self._all_connections.append(conn)
        self._free_connections.append(conn)

    def _forget_connection(self, conn: Connection) -> None:
        """
        - Close connection if open
        - Remove from internal structures
        """
        close_silently(conn)
        try:
            self._free_connections.remove(conn)
        except ValueError:
            pass
        self._all_connections.remove(conn)

    def _getconn(self) -> Connection:
        """
        Get new connection
        """
        if not self._free_connections:
            if len(self._all_connections) == self.maxconn:
                raise PoolExhaustedError
            self._connect()

        while self._free_connections:
            conn = self._free_connections.popleft()
            try:
                with conn.cursor() as cursor:
                    cursor.execute('SELECT 1')
                    cursor.fetchone()
                    return conn
            except psycopg2.Error:
                self._putconn(conn, close=True)

        self._connect()
        return self._free_connections.popleft()

    def getconn(self) -> Connection:
        """
        Get new connection
        """
        with self._lock:
            return self._getconn()

    @contextmanager
    def c_getconn(self) -> Generator[Connection, None, None]:
        """
        Get new connection and return it on exit from with block
        """
        conn = self.getconn()
        try:
            yield conn
        finally:
            self.putconn(conn)

    def _putconn(self, conn: Connection, close: bool = False) -> None:
        """
        Return connection to pool
        """
        if conn not in self._all_connections:
            conn_host = None
            try:
                conn_host = conn.get_dsn_parameters()['host']
            except psycopg2.Error:
                pass
            my_host = self._connect_kwargs.get('host')
            # For broken connection psycopg2 can not return its host
            if conn_host is not None and conn_host != my_host:
                raise UnexpectedConnectionError(
                    'Got unknown connection, probably to another host. '
                    f'My host is {my_host}, connection host is {conn_host}'
                )
            close_silently(conn)
            return

        is_overflow = len(self._all_connections) > self.minconn
        if is_overflow or close:
            self._forget_connection(conn)
            return

        can_be_reused = refresh_connection(conn)
        if can_be_reused:
            self._free_connections.append(conn)
            return

        self._forget_connection(conn)

    def putconn(self, conn: Connection, close=False) -> None:
        """
        Return connection to pool
        """
        with self._lock:
            self._putconn(conn, close)

    def closeall(self) -> None:
        """
        Close all connections
        """
        with self._lock:
            for conn in self._all_connections[:]:
                self._forget_connection(conn)

    def recreate(self) -> None:
        """
        Recreate all free connections
        """
        with self._lock:
            for conn in list(self._free_connections):
                # create new connection first,
                # in case if it fails we don't forget to remove old connection
                self._connect()
                self._forget_connection(conn)

    def grow(self) -> None:
        """
        Open more connections if need it
        """
        if len(self._all_connections) >= self.minconn:
            return
        with self._lock:
            # Our getconn drain connections (ping and drop unusable),
            # so if our database is unavailable at that point we may have less than `minconn` connections.
            # If we have more then minconn connections, it's not a problem, cause: list(range(-42)) == []
            for _ in range(self.minconn - len(self._all_connections)):
                self._connect()
