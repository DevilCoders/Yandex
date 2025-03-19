"""KiKiMR client"""

_KIKIMR_CLIENT_UNKNOWN = 0
_KIKIMR_CLIENT_YC = 1  # Own driver over protocol version 1, deprecated
KIKIMR_CLIENT_YDB = 2  # Python kikimr driver

# Add log modules for debug ydb sdk
YDB_SDK_DEBUG_LOG_MODULES = ("ydb.connection", "ydb._sp_impl.SessionPoolImpl")

import threading
import traceback

import abc
import atexit
import itertools
import os
import re
import time

from yc_common.exceptions import Error, LogicalError

# for prevent protobuf conflicts from kikimrclient protobufs and YDB own protobuf may import only one of them
_YDB_KIKIMR_CLIENT = 2


# defining it before importing stuff where it is used to avoid circular imports
def client_version():
    return _YDB_KIKIMR_CLIENT


from concurrent.futures import Future, TimeoutError as FutureTimeoutError, ThreadPoolExecutor
from decimal import Decimal
from enum import Enum
from threading import Lock, Thread as SysThread
from typing import Any, TypeVar, Type, Union, Tuple, List, Dict, Iterable, Sequence, Callable

import grpc
import weakref, sys

if _YDB_KIKIMR_CLIENT == KIKIMR_CLIENT_YDB:
    import ydb
    from kikimr.public.sdk.python.iam import auth as ydb_auth

    # make stub class hierarchy for work type hints
    class _StubClass:
        pass

    kqp_pb2 = minikql_pb2 = EPathTypeDir = EPathTypeTable = ESchemeOpRmDir = ESchemeOpMkDir= _StubClass
    ESchemeOpCreateTable = ESchemeOpCreateTable = TTableDescription = TQueryResponse = TYqlRequest = _StubClass
    TYqlResponse = TSchemeDescribe = TSchemeOperation = TSchemeOperationStatus = TGRpcServerStub = _StubClass
    TSeverityIds = _StubClass
else:
    from ydb.core.protos import kqp_pb2, minikql_pb2
    from ydb.core.protos.flat_scheme_op_pb2 import EPathTypeDir, EPathTypeTable, ESchemeOpRmDir, \
        ESchemeOpMkDir, ESchemeOpCreateTable, TTableDescription
    from ydb.core.protos.kqp_pb2 import TQueryResponse
    from ydb.core.protos.msgbus_pb2 import TYqlRequest, TYqlResponse, TSchemeDescribe, TSchemeOperation, TSchemeOperationStatus
    from ydb.core.protos.grpc_pb2_grpc import TGRpcServerStub
    from yql.public.issue.protos.issue_severity_pb2 import TSeverityIds

    # make stub class hierarchy for work type hints
    class ydb:
        class convert:
            class ResultSet:
                pass
        class Driver:
            pass
        class Error:
            pass
        class SerializableReadWrite:
            pass
        class table:
            class AbstractTransactionModeBuilder:
                pass
            class Session:
                pass
            class TxContext:
                pass
        class TableSchemeEntry:
            pass


    class ydb_auth:
        class MetadataUrlCredentials:
            pass


from yc_common.constants import SERVICE_REQUEST_TIMEOUT

from yc_common.backoff import gen_exponential_backoff
from yc_common import constants, logging, context
from yc_common.clients.kikimr.sql import SqlOrder, SqlLimit, Variable, QueryTemplate
from yc_common.clients.kikimr import metrics, sql
from yc_common.clients.kikimr.kikimr_types import KikimrDataType, table_spec_type_to_internal_type

from yc_common.misc import safe_zip, Thread
from yc_common.models import Model, DECIMAL_SCALE

from .config import KikimrEndpointConfig, get_database_config
from .exceptions import ValidationError, QueryError, PathNotFoundError, NotDirectoryError, \
    NotTableError, DirectoryIsNotEmptyError, TableNotFoundError, ColumnNotFoundError, ColumnAlreadyExistsError, PreconditionFailedError, RetryableError, \
    RetryableConnectionError, RetryableRequestTimeoutError, TransactionLocksInvalidatedError, UnretryableConnectionError, \
    YdbDeadlineExceeded, BadRequest, CompileQueryError, BadSession, KikimrError, YdbOverloaded, YdbUnavailable, \
    YdbCreateSessionTimeout
from .util import retry_idempotent_kikimr_errors
from .util import strip_table_name_from_row, ColumnStrippingStrategy  # For backward compatibility

_FUTURE_LOG_INTERVAL = 1  # Log every _FUTURE_LOG_INTERVAL seconds for wait result in _wrap_future_ functions
_ADDITIONAL_FORCE_YDB_TIMEOUT = 10  # If ydb doesn't stop operation after timeout - stop wait it by client code after this seconds

_TAny = TypeVar("_TAny")

log = logging.get_logger(__name__)

_MAX_RECEIVE_MESSAGE_LENGTH = 50 * constants.MEGABYTE
"""Matches current internal KiKiMR limits on max query response size."""

# AlterTable : kikimr.[/Root/ycloud/vagrant/d2ed69c8-3cc9-434d-9a69-653dc2ae82ec/compute_az/test_table]
# Column: "name" already exists. - _KIKIMR_CLIENT_YC
# Column: \"t2\" already exists  - KIKIMR_CLIENT_YDB
_COLUMN_ALREADY_EXIST_ERROR_RE = re.compile(
    r"""column\s*?:\s+\\?".+?\\?"\s+already\s+exists""", re.IGNORECASE)

# AlterTable : kikimr.[/Root/ycloud/vagrant/d2ed69c8-3cc9-434d-9a69-653dc2ae82ec/compute_az/test_table]
# Column: "name" does not exists. - _KIKIMR_CLIENT_YC
# Column: \"t3\" does not exists  - KIKIMR_CLIENT_YDB
_COLUMN_DOES_NOT_EXIST_ERROR_RE = re.compile(
    r"""column\s*?:\s+\\?".+?\\?"\s+does\s+not\s+exists""", re.IGNORECASE)


_YDB_DRIVERS = {}  # type: Dict[Tuple, "_YdbDriverDescription"]
_YDB_DRIVERS_LOCK = Lock()
_YDB_EXIT_CLEAN = False
_YDB_SESSIONS_IN_USE = set()  # type: Set[int]  # set of id sessions objects in ok
_YDB_SESSIONS_IN_USE_CHECK = False

_YDB_STACKTRACE_ENABLED = {}  # type: Dict[Tuple,bool]
_YDB_GET_PUT_SESSION_LOG_ENABLED = {}  # type: Dict[Tuple,bool]
_YDB_SESSIONS_DEBUG_INFO = {}  # type: Dict[int, _SessionDebugInfo]



class _SessionDebugInfo():
    MAX_TRACES = 20

    def __init__(self, internal_session_id, config):
        # Session returned from session get and can be used.
        # False mean - session returned to pool and can't use
        self.have_finalizer = False
        self.__stacktraces = []  # type: List[str]  # (time, stacktrace)
        self.internal_id = internal_session_id
        self.db_config = config  # type: KikimrEndpointConfig

    def add_get_trace(self):
        self.__append_stack('get')

    def add_put_trace(self):
        self.__append_stack('put')

    def __append_stack(self, label):
        ctx = context.get_context().to_dict()
        stack = traceback.extract_stack()
        stack_s = "\n".join(traceback.format_list(stack))
        t = time.strftime("%Y.%m.%d %H:%M:%S")
        self.__stacktraces.append((t, label, stack_s, ctx))
        if len(self.__stacktraces) > _SessionDebugInfo.MAX_TRACES:
            self.__stacktraces = self.__stacktraces[len(self.__stacktraces) // 2:]

    def log_stack(self, message,  *args, level=logging.DEBUG):
        stacktraces = self.__stacktraces.copy()
        stacktraces.reverse()  # newer first
        message += "Ydb session stacktraces: %s"
        log.log(level, message, *(args + (stacktraces,)), )

    def log_last_get(self, message,  *args, level=logging.DEBUG, log_in_item_context=False):
        last_get = None
        for item in reversed(self.__stacktraces):
            if item[1] == "get":
                last_get = item
                break
        if message != "" and not message.endswith(" "):
            message += " "
        message += "Last get stacktrace: %s"
        if log_in_item_context:
            with context.switch_context(**last_get[3]):
                log.log(level, message, *(args + (last_get,)))
        else:
            log.log(level, message, *(args + (last_get,)))


class TransactionMode(Enum):
    SERIALIZABLE_READ_WRITE = "SERIALIZABLE_READ_WRITE"
    ONLINE_READ_ONLY_CONSISTENT = "ONLINE_READ_ONLY_CONSISTENT"
    ONLINE_READ_ONLY_INCONSISTENT = "ONLINE_READ_ONLY_INCONSISTENT"
    STALE_READ_ONLY = "STALE_READ_ONLY"


class _YdbDriverDescription:
    def __init__(self, driver: ydb.Driver, config: KikimrEndpointConfig):
        self.driver = driver
        self.config = config
        self.pool = None  # type: ydb.SessionPool
        self.pool_lock = Lock()
        self.pool_size = 0


def _ydb_clean_sessions():
    global _YDB_EXIT_CLEAN

    with _YDB_DRIVERS_LOCK:
        _YDB_EXIT_CLEAN = True

        for driver_description in _YDB_DRIVERS.values():
            # stop session pool, when it wil available in ydb SDK

            try:
                _wrap_ydb_db_stop(driver_description.driver, driver_description.config.ydb_stop_timeout)
            except Exception as e:
                log.error("Stop YDB driver failed: %s", e)

        if len(_YDB_SESSIONS_DEBUG_INFO) > 0 and len(_YDB_SESSIONS_IN_USE) > 0 and \
                len(_YDB_GET_PUT_SESSION_LOG_ENABLED) > 0:
            log.debug("Sessions aren't returned to pools: %s", len(_YDB_SESSIONS_DEBUG_INFO))

            keys = list(_YDB_SESSIONS_DEBUG_INFO.keys())
            for key in keys:
                if key not in _YDB_SESSIONS_IN_USE:
                    continue

                debug = _YDB_SESSIONS_DEBUG_INFO.get(key)
                if debug is None:
                    continue
                log.debug("YDB session key: %s", key)
                debug.log_last_get("Clean ydb session.", level=logging.DEBUG)

        _YDB_DRIVERS.clear()


atexit.register(_ydb_clean_sessions)


def _ydb_key_from_database_config(config: KikimrEndpointConfig) -> Tuple:
    return config.host, config.database


def _ydb_driver_get_description(config: KikimrEndpointConfig) -> _YdbDriverDescription:
    global _YDB_STACKTRACE_ENABLED, _YDB_SESSIONS_IN_USE_CHECK

    key = _ydb_key_from_database_config(config)

    try:
        return _YDB_DRIVERS[key]  # Safe because GIL
    except KeyError:
        pass

    with _YDB_DRIVERS_LOCK:
        if key not in _YDB_DRIVERS:
            auth_token = None
            if config.ydb_auth_token_file is not None:
                with open(config.ydb_auth_token_file) as f:
                    auth_token = f.read().rstrip()

            meta_credentials = None
            if config.ydb_token_from_metadata is not None:
                if config.ydb_token_from_metadata in ("", "default-url"):
                    meta_credentials = ydb_auth.MetadataUrlCredentials()
                else:
                    meta_credentials = ydb_auth.MetadataUrlCredentials(metadata_url=config.ydb_token_from_metadata)

            root_certificates = None
            if config.root_ssl_cert_file is not None:
                with open(config.root_ssl_cert_file, "rb") as f:
                    root_certificates = f.read()

            driver_description = _YdbDriverDescription(ydb.Driver(ydb.ConnectionParams(
                config.host, config.database, auth_token=auth_token, credentials=meta_credentials,
                root_certificates=root_certificates,
            )), config)
            try:
                _wrap_ydb_db_wait(config, driver_description.driver, config.ydb_connection_timeout,
                                  config.warning_request_time)
            except (RetryableConnectionError, ydb.ConnectionError) as ex:
                log.error("Db wait error, stop background discovery: %s.", ex)
                driver_description.driver.stop(timeout=1)
                raise _ydb_convert_exception(ex, 'driver_wait', config)
            _YDB_DRIVERS[key] = driver_description

            if config.enable_trace_ydb_session_leak:
                _YDB_STACKTRACE_ENABLED[key] = True
                _YDB_SESSIONS_IN_USE_CHECK = True

        return _YDB_DRIVERS[key]


def _ydb_driver_get(config: KikimrEndpointConfig) -> ydb.Driver:
    return _ydb_driver_get_description(config).driver


def _ydb_session_get(config: KikimrEndpointConfig) -> ydb.table.Session:
    if _YDB_EXIT_CLEAN:
        raise LogicalError("Get YDB session while shutdown.")

    key = _ydb_key_from_database_config(config)
    driver_description = _ydb_driver_get_description(config)

    if config.disable_pool:
        try:
            session = _wrap_ydb_session_create(driver_description.driver.table_client.session(), config.request_timeout,
                                               _connection_retry_timeout=config.retry_timeout)
        except ydb.Error as ex:
            raise _ydb_convert_exception(ex, 'create_session', config)
        # log.debug("Ignore session pool, because config.")
    else:
        if driver_description.pool is None:
            with driver_description.pool_lock:
                if driver_description.pool is None:
                    driver_description.pool = ydb.SessionPool(driver_description.driver,
                            size=config.ydb_session_pool_initial_size,
                            workers_threads_count=config.ydb_session_worker_count)
                    driver_description.pool_size = config.ydb_session_pool_initial_size

        try:
            session = driver_description.pool.acquire(timeout=config.ydb_wait_session_timeout)  # type: ydb.table.Session
            driver_description.pool_size -= 1
        except ydb.SessionPoolEmpty:
            log.warn("No sessions in pool. Create new session. Current pool size by counter: %s",
                     driver_description.pool_size)
            try:
                session = _wrap_ydb_session_create(driver_description.driver.table_client.session(),
                                config.request_timeout, _connection_retry_timeout=config.retry_timeout)
            except FutureTimeoutError:
                raise YdbCreateSessionTimeout("Timeout session pool empty")
            except ydb.Error as ex:
                raise _ydb_convert_exception(ex, 'create_session_while_pool_empty', config)

        except ydb.Error as ex:
            raise _ydb_convert_exception(ex, 'get_session_from_pool', config)

    internal_id = id(session)
    debug = None
    if _YDB_GET_PUT_SESSION_LOG_ENABLED.get(key):
        log.debug("Get ydb_session session_id: '%s', id(session): '%s'", session.session_id, id(session))

    if _YDB_STACKTRACE_ENABLED.get(key):
        debug = _YDB_SESSIONS_DEBUG_INFO.get(internal_id, _SessionDebugInfo(internal_id, config))
        debug.add_get_trace()
        if not debug.have_finalizer:
            weakref.finalize(session, _ydb_session_finalizer, id(session), session_log_enabled=_YDB_GET_PUT_SESSION_LOG_ENABLED.get(key))
            debug.have_finalizer = True
        _YDB_SESSIONS_DEBUG_INFO[internal_id] = debug

        if _YDB_SESSIONS_IN_USE_CHECK:
            if internal_id in _YDB_SESSIONS_IN_USE:
                if debug is not None:
                    debug.log_stack("Double get session without put it.", level=logging.ERROR)
                raise LogicalError("Double get session without put it.")
            else:
                _YDB_SESSIONS_IN_USE.add(internal_id)

    metrics.ydb_connections_pool.labels(config.root).inc()
    return session


def _ydb_session_renew(config:KikimrEndpointConfig, old_session: ydb.table.Session, exception) -> ydb.table.Session:
    new_session = _ydb_session_get(config)
    _ydb_session_put(config, old_session, exception)
    return new_session


def _ydb_session_reset(session: ydb.table.Session):
    session.reset()


class _SessionContext:
    def __init__(self, config: KikimrEndpointConfig):
        self.__config = config
        self.__entered = None  # type: ydb.table.Session

    def __enter__(self) -> ydb.table.Session:
        if self.__entered is not None:
            raise LogicalError("Double get session from same session context")
        self.__entered = _ydb_session_get(self.__config)
        return self.__entered

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.__entered is None:
            raise LogicalError("Double exit from session context")
        _ydb_session_put(self.__config, self.__entered, exc_val)
        self.__entered = None


def _ydb_session_finalizer(session_internal_id: int, session_log_enabled: bool):
    if session_log_enabled:
        log.debug("Clean debug info session_internal_id: '%s'", session_internal_id)

    debug = _YDB_SESSIONS_DEBUG_INFO.pop(session_internal_id, None)

    if debug is None and session_log_enabled:
        log.error("Call debug cleaner without debug info.")

    if _YDB_SESSIONS_IN_USE_CHECK and session_internal_id in _YDB_SESSIONS_IN_USE:
        if debug is not None:
            metrics.ydb_leak_sessions.labels(debug.db_config.root).inc()
            debug.log_last_get("Leak ydb session.", level=logging.ERROR, log_in_item_context=True)
        _YDB_SESSIONS_IN_USE.discard(session_internal_id)


def _ydb_session_must_in_use(session: ydb.table.Session):
    if not _YDB_SESSIONS_IN_USE_CHECK:
        return

    internal_id = id(session)
    if internal_id in _YDB_SESSIONS_IN_USE:
        return

    try:
        debug = _YDB_SESSIONS_DEBUG_INFO[internal_id]
        debug.log_stack("Session must be in use.", level=logging.ERROR)
    except KeyError:
        log.debug("Haven't debug info: session internal id: %d", internal_id)

    raise LogicalError("Use ydb session out of get/put context.")


def _ydb_session_context(config: KikimrEndpointConfig) -> _SessionContext:
    return _SessionContext(config)


# Pre put ydb_session_check and reset if need.
def _ydb_session_reset_if_hang(session: ydb.table.Session, db_config: KikimrEndpointConfig, exception_val: Exception):
    # If put sessions without errors - no need reset
    if exception_val is None:
        return session

    if session.session_id is None:
        _ydb_session_reset(session)
        return session


    # Some exceptions - mean that no pending query and session may return to poll as is.
    can_have_pending_query = True

    # Known exception, that can't hung up queries on server side
    if isinstance(exception_val, TransactionLocksInvalidatedError):
        can_have_pending_query = False

    if not can_have_pending_query:
        return session

    # Default - reset session if it put after error becouse it can has pending query.
    log.info("Close and reset ydb session '%s' because exception: '%s'.", session.session_id, exception_val)
    try:
        _wrap_ydb_session_delete(session, db_config.request_timeout, _connection_retry_timeout=db_config.ydb_connection_timeout)
    except ydb.Error as e:
        # log close session error and continue work
        log.error("Error while close server side session: '%s'", str(e))
    _ydb_session_reset(session)
    return session


def _ydb_session_put(config: KikimrEndpointConfig, session: ydb.table.Session, exception_value):
    try:
        if session is None:
            # session may be None if driver can't connect to YDB server
            if isinstance(exception_value, (RetryableConnectionError, YdbCreateSessionTimeout, ydb.DeadlineExceed)):
                return
            raise LogicalError()

        metrics.ydb_connections_pool.labels(config.root).dec()

        internal_id = id(session)
        if _YDB_SESSIONS_IN_USE_CHECK:
            _ydb_session_must_in_use(session)
            _YDB_SESSIONS_IN_USE.discard(internal_id)

        key = _ydb_key_from_database_config(config)
        if _YDB_GET_PUT_SESSION_LOG_ENABLED.get(key):
            log.debug("Put ydb_session session_id: '%s', id(session): '%s', refcount: %s", session.session_id, id(session),
                      sys.getrefcount(session)-1)

        if _YDB_STACKTRACE_ENABLED.get(key):
            debug = _YDB_SESSIONS_DEBUG_INFO.get(internal_id)
            if debug is None:
                debug = _SessionDebugInfo(internal_id, config)
                debug.add_put_trace()
                log.error("Put session without debug info.")
                debug.log_stack(logging.ERROR)
            else:
                debug.add_put_trace()

        session = _ydb_session_reset_if_hang(session, config, exception_value, )

        driver_description = _ydb_driver_get_description(config)

        if config.disable_pool or driver_description.pool_size >= config.ydb_session_pool_max_size:
            if session.session_id is not None:
                _wrap_ydb_session_delete(session, config.request_timeout, _connection_retry_timeout=config.retry_timeout)
            return

        driver_description.pool.release(session)
        driver_description.pool_size += 1

    except ydb.Error as ex:
        return _ydb_convert_exception(ex, 'ydb_session_put', config)


class _UnexpectedResponseError(ValidationError):
    def __init__(self, details, message="Got an unexpected response from KiKiMR.", *args):
        message += " Details: `{}`."
        args = args + (details,)
        super().__init__(message, *args)


class _UnexpectedResponseCountError(_UnexpectedResponseError):
    def __init__(self, details, expected, got):
        super().__init__(details, "Got an unexpected response count from KiKiMR. Expected: {} Got: {}.",
                         expected, got)


class _TResponseStatus:
    UNKNOWN = 0
    """Status unknown, must not be seen"""

    OK = 1
    """Request complete with full success"""

    ERROR = 128
    """Request failed because of client-side error"""

    IN_PROGRESS = 129
    """Request accepted and in progress"""

    TIMEOUT = 130
    """Request failed due to timeout, exact status unknown"""

    NOT_READY = 131
    """Not yet ready to process requests, try later"""

    ABORTED = 132
    """Request aborted, particular meaning depends on context"""

    INTERNAL_ERROR = 133
    """Something unexpected happend on server-side"""

    REJECTED = 134
    """Request rejected for now, try later"""


class _KikimrErrorCodes:
    PATH_NOT_FOUND = ("Path not found", "Path is not found")
    DIRECTORY_IS_NOT_EMPTY = "Can't remove not empty directory"


class KikimrTableSpec:
    @classmethod
    def from_kikimr_spec(cls, spec) -> "KikimrTableSpec":
        if isinstance(spec, TTableDescription):  # for driver _KIKIMR_CLIENT_YC
            return cls(
                columns={c.Name: c.Type for c in spec.Columns},
                primary_keys=spec.KeyColumnNames)
        elif isinstance(spec, ydb.TableSchemeEntry):
            return cls(
                columns={c.name: table_spec_type_to_internal_type(c.type) for c in spec.columns},
                primary_keys=spec.primary_key
            )

    def __init__(self, columns: Dict[str, str], primary_keys: List[str]):
        def normalize(value: str) -> str:
            return value.strip().lower()

        def normalize_type(value: str) -> str:
            value = value.strip()
            # Fix for decimal type that must be wrapped in backticks.
            if value[0] == "`":
                return "`" + value[1].upper() + value[2:].lower()
            if value == "Decimal":
                return KikimrDataType.DECIMAL
            return value[0].upper() + value[1:].lower()

        self.__columns = {normalize(k): normalize_type(v) for k, v in columns.items()}
        self.__primary_keys = [normalize(k) for k in primary_keys]

        if not (self.__primary_keys and self.__columns):
            raise LogicalError("You must specify at least one primary key and column.")
        if not all(pk in self.__columns for pk in self.__primary_keys):
            raise LogicalError("Not all primary keys are listed in the column spec.")
        if not all(kdt in KikimrDataType.ALL for kdt in self.__columns.values()):
            raise LogicalError("Unknown data types are used.")

    @property
    def columns(self):
        return self.__columns

    @property
    def primary_keys(self):
        return self.__primary_keys

    def __eq__(self, other):
        return isinstance(other, self.__class__) and \
               self.__columns == other.__columns and \
               self.__primary_keys == other.__primary_keys

    def __ne__(self, other):
        return not self == other

    def __repr__(self):
        return "{}, PRIMARY KEY ({})".format(
            ", ".join("{} {}".format(k, v) for k, v in self.__columns.items()),
            ", ".join(self.__primary_keys))


class KikimrTable:
    def __init__(self, database_id: str, table_name: str, table_spec: KikimrTableSpec,
                 model=None,  # At this time is used only for self-descriptive code reasons
                 database_config: KikimrEndpointConfig = None):

        # FIXME: remove this check later after transition period
        if not isinstance(table_spec, KikimrTableSpec):
            raise LogicalError("Now KikimrTable spec need to be KikimrTableSpec class instance.")

        self._database_id = database_id
        self._table_name = table_name
        self.__table_spec = table_spec

        # Cached values
        self.__database_config = database_config
        self.__path = None

    @property
    def client(self):
        return _KikimrSimpleConnection(self._database_id, self._database_config, scope=self._scope)

    @property
    def client_ydb(self):
        """
        Return new client - need for experiments with new client only.
        The property will remove after turn on new client by default.
        :return:
        """
        return _KikimrSimpleConnection(self._database_id, self._database_config, scope=self._scope,
                                       ydb_client=KIKIMR_CLIENT_YDB)

    @property
    def database_id(self):
        return self._database_id

    @property
    def name(self):
        return self._table_name

    @property
    def path(self):
        path = self.__path

        if path is None:
            self.__path = path = \
                self._database_config.get_root_path().rstrip("/") + "/" + self._table_name

        for x in range(path.count("/..")):
            path = re.sub("/[^/]+/../", "/", path)

        return path

    def is_exists(self):
        # FIXME: optimize this query (maybe with some DescribeScheme request for _sql_name or something else)
        dir_content = self.client.list_directory(self._database_config.get_root_path())

        return self._table_name in {entry["name"] for entry in dir_content if not entry["directory"]}

    def initial_spec(self) -> KikimrTableSpec:
        return self.__table_spec

    def current_spec(self) -> KikimrTableSpec:
        return self.client.get_table_spec(self.path)

    def create(self):
        self.client.query("CREATE TABLE $table ({})".format(
            self.__table_spec), transactionless=True, idempotent=True)

    def drop(self, only_if_exists=False):
        try:
            self.client.query("DROP TABLE $table", transactionless=True, idempotent=only_if_exists)
        except TableNotFoundError:
            if not only_if_exists:
                raise

    def add_column(self, name, data_type):
        self._alter_table("ADD COLUMN {} {}".format(name, data_type))

    def drop_column(self, name, only_if_exists=False):
        try:
            self._alter_table("DROP COLUMN {}".format(name), idempotent=only_if_exists)
        except ColumnNotFoundError:
            if not only_if_exists:
                raise

    def column_type(self, column_name) -> str:
        return self.__table_spec.columns[column_name]

    def var(self, column_name, value) -> Variable:
        return Variable(value, self.column_type(column_name))

    def _alter_table(self, query, idempotent=False):
        self.client.query("ALTER TABLE $table {}".format(query), transactionless=True, idempotent=idempotent)

    @property
    def _scope(self):
        return _KikimrBaseConnectionScope(self)

    @property
    def _sql_name(self):
        return "[{}]".format(self.path)

    @property
    def _database_config(self):
        database_config = self.__database_config

        if database_config is None:
            self.__database_config = database_config = get_database_config(self._database_id)

        return database_config

    def drop_cache(self):
        self.__path = None
        self.__database_config = None


class KikimrCursorSpec:
    def __init__(self, table: KikimrTable, table_alias=None, cursor_limit=None, cursor_value=None, join_column=None,
                 reverse_order=False):
        self.table = table
        self.table_alias = table_alias
        self.cursor_limit = cursor_limit
        self.cursor_value = cursor_value
        self.join_column = join_column
        self.reverse_order = reverse_order

    @property
    def columns(self):
        return self.table.initial_spec().primary_keys

    def get_sql_order(self) -> SqlOrder:
        order = SqlOrder(self.columns)

        if self.reverse_order:
            order = order.reverse()

        return order


class _YdbResponse:
    def __init__(self):
        # noinspection PyProtectedMember
        self.results = []  # type: List[ydb.convert._ResultSet]


class _PreparedQuery:
    def __init__(self, session_id, query_text, prepared_query):
        self._session_id = session_id
        self._text = query_text
        self._prepared = prepared_query


class _SelectPager(abc.ABC):
    def __init__(self, max_select_rows):
        super().__init__()
        self.max_select_rows = max_select_rows

    @abc.abstractmethod
    def get_rows_from_page(self, results) -> List:
        pass

    @abc.abstractmethod
    def is_last_page(self, results) -> bool:
        pass

    @abc.abstractmethod
    def get_cursor_values(self, results, render_column, cursor_columns) -> List:
        pass

    @classmethod
    def _get_cursor_values_from_row(cls, row, strip_table_from_columns, model, render_column, cursor_columns):
        cursor_values = []
        for column in cursor_columns:
            if model is None and not strip_table_from_columns:
                output_column_name = render_column(column)
            else:
                output_column_name = column

            if output_column_name not in row:
                raise LogicalError("Invalid paged query: The result doesn't contain {!r} column.",
                                   output_column_name)

            cursor_value = row[output_column_name]
            if cursor_value is None:
                raise Error("Error during paged query processing: Got NULL {!r} column.", output_column_name)

            cursor_values.append(cursor_value)
        return cursor_values


class _KikimrBaseConnectionScope:
    def __init__(self, default_table: KikimrTable=None):
        self.default_table = default_table
        self.table_names = {}

        if self.default_table is not None:
            self.table_names["table"] = default_table._sql_name


class _KikimrBaseConnection(abc.ABC):
    """
    It has internal ydb session pool.
    Should use with context manager for prevent sessions leak.
    """
    _COMMIT_COMMAND = "COMMIT"
    _ROLLBACK_COMMAND = "ROLLBACK"

    def __init__(self, database_id: str, database_config: KikimrEndpointConfig, scope=None,
                 request_timeout=None, retry_timeout: Union[int, float]=None, scheme_operation_timeout=None, *, ydb_client=None
                 ):
        """
        :param session: use external session instead of get own from pool. Not release session when close self.
        :param ydb_client: version of kikimr client. User KIKIMR_CLIENT_YDB for force new ydb_client.
               Experemental stage only, then parameter will be remove.
        """
        self._database_id = database_id
        self._database_config = database_config
        self._request_timeout = request_timeout if request_timeout is not None \
            else self._database_config.request_timeout
        self._scheme_operation_timeout = scheme_operation_timeout if scheme_operation_timeout is not None \
            else self._database_config.scheme_operation_timeout
        self.__session_timeout = self._request_timeout
        self._retry_timeout = retry_timeout if request_timeout is not None else self._database_config.retry_timeout  # type: int
        self._grpc_connection = _GrpcConnection(database_config.host)
        self._scope = _KikimrBaseConnectionScope() if scope is None else scope
        self.__can_hide_bad_session = True
        self._autocommit = False  # auto commit all queries. Workaround for https://st.yandex-team.ru/KIKIMR-6103

        if _YDB_KIKIMR_CLIENT == _KIKIMR_CLIENT_UNKNOWN:
            LogicalError("Need env variable: YDB_CLIENT_VERSION")

        self._ydb_client = _YDB_KIKIMR_CLIENT

        self._ydb_session_val = None  # type: ydb.table.Session

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        pass

    def __ydb_session_maker(method: _TAny) -> _TAny:
        """
        Gurantee for self have ydb_session - get if from pool if need.
        If session was got from pool - it was release to pool before exit
        """
        def __ydb_session_maker_wrapper(self: "_KikimrBaseConnection", *args, **kwargs):
            ex = None
            if self._ydb_client == KIKIMR_CLIENT_YDB and self._ydb_session_val is None:
                try:
                    self._ydb_session_val = _ydb_session_get(self._database_config)
                    return method(self, *args, **kwargs)
                except (ydb.Error, KikimrError) as e:
                    ex = e
                    raise
                finally:
                    # not use session context, becouse session can be replaced inside method
                    # while handling errors
                    _ydb_session_put(self._database_config, self._ydb_session_val, ex)
                    self._ydb_session_val = None
            else:
                return method(self, *args, **kwargs)

        return __ydb_session_maker_wrapper

    def table_scope(self, *args: KikimrTable, **kwargs: KikimrTable):
        if len(args) == 1 and not kwargs:
            tables = args
            scope = _KikimrBaseConnectionScope(tables[0])
        elif not args and kwargs:
            tables = tuple(kwargs.values())
            scope = _KikimrBaseConnectionScope()
            scope.table_names = {table_name + "_table": table._sql_name for table_name, table in kwargs.items()}
        else:
            raise LogicalError()

        for table in tables:
            if table._database_id != self._database_id:
                raise ValidationError(
                    "Unable to set {!r} table scope for connection: database mismatch ({} instead of {}).",
                    table._table_name, table._database_id, self._database_id)

        class TableScopeContext:
            def __init__(self, connection: _KikimrBaseConnection):
                self.connection = connection

            def __enter__(self):
                self.__orig_scope = self.connection._scope
                self.connection._scope = scope
                return self.connection

            def __exit__(self, exc_type, exc_val, exc_tb):
                self.connection._scope = self.__orig_scope

        return TableScopeContext(self)

    def with_table_scope(self, *args, **kwargs):
        """
        Use when need apply scope with apply __enter__ implicit: for prevent deep nesting of with operators.
        __exit__ will apply NEVER.
        For example: to apply context manager to connection (transaction) or for call method of connection

        Examples:
        1.  with task.tx().with_table_scope(instances=instances_table,
                compute_node_index=instance_compute_node_index_table) as tx:

            Description:
            will create new transaction in task.tx(), then apply scope implicit and return transaction object.
            Context manager will call __enter__ and __exit__ for transaction object.


        2.  with az_db().with_table_scope(
                instances=instances_table,
                compute_node_index=instance_compute_node_index_table
            ).transaction() as tx:

            Description:
            az_db() creates simple connection, then implicitly applies scope and returns simple connection.
            Then run transaction() for connection object.
            Context manager will call __enter__ and __exit__ for transaction object.
        """
        return self.table_scope(*args, **kwargs).__enter__()

    def _scheme_describe_request(self, path):
        query = "SchemeDescribe"
        request = TSchemeDescribe()
        request.Path = path

        response = self._send_request("SchemeDescribe", request, idempotent=True)

        if response.Status != _TResponseStatus.OK:
            # TODO: Should we use _handle_error() here?
            error_code, error_message = _get_error(response.ErrorReason)

            # FIXME: maybe KqpStatus checking is not needed?
            if error_code in _KikimrErrorCodes.PATH_NOT_FOUND or getattr(response, "KqpStatus", None) == kqp_pb2.STATUS_SCHEME_ERROR:
                raise PathNotFoundError(query, error_message)
            else:
                raise QueryError(query, error_message)

        return response

    def _describe_table(self, path):
        if self._ydb_client == KIKIMR_CLIENT_YDB:
            try:
                with _ydb_session_context(self._database_config) as session:
                    response = _wrap_ydb_describe_table(session, path, self._scheme_operation_timeout,
                                                        self._database_config.warning_request_time,
                                                        _connection_retry_timeout=self._retry_timeout)
                if response.type != ydb.SchemeEntryType.TABLE:
                    raise NotTableError("Describe table '{}'".format(path), "The specified path is not a table.")
                return response
            except ydb.SchemeError as ex:
                raise _ydb_convert_exception(ex, "Describe table '{}'".format(path), self._database_config)
        else:
            query = "SchemeDescribe"
            response = self._scheme_describe_request(path)

            if response.PathDescription.Self.PathType != EPathTypeTable:
                raise NotTableError(query, "The specified path is not a table.")

        return response

    def get_table_spec(self, path) -> KikimrTableSpec:
        response = self._describe_table(path)
        if self._ydb_client == KIKIMR_CLIENT_YDB:
            return KikimrTableSpec.from_kikimr_spec(response)
        else:
            return KikimrTableSpec.from_kikimr_spec(response.PathDescription.Table)

    def list_directory(self, path) -> List[Dict[str, Union[str, bool]]]:
        children = []

        if self._ydb_client == KIKIMR_CLIENT_YDB:
            try:
                query = "Describe directory {!r}".format(path)
                response = _wrap_ydb_list_directory(self._database_config, path, self._scheme_operation_timeout,
                                                    self._database_config.warning_request_time,
                                                    _connection_retry_timeout=self._retry_timeout)
                if response.type != ydb.SchemeEntryType.DIRECTORY:
                    raise NotDirectoryError(query, "The specified path is not a directory.")

                for child in response.children:
                    if not child.name:
                        raise QueryError(query, "Got a child with an empty name.")

                    children.append({
                        "name": child.name,
                        "directory": child.type == ydb.SchemeEntryType.DIRECTORY,
                    })
            except ydb.SchemeError as ex:
                raise _ydb_convert_exception(ex, "Describe directory '{}'".format(path), self._database_config)
        else:
            query = "SchemeDescribe"
            response = self._scheme_describe_request(path)

            if response.PathDescription.Self.PathType != EPathTypeDir:
                raise NotDirectoryError(query, "The specified path is not a directory.")

            for child in response.PathDescription.Children:
                if not child.Name:
                    raise QueryError(query, "Got a child with an empty name.")

                children.append({
                    "name": child.Name,
                    "directory": child.PathType == EPathTypeDir
                })

        return children

    def create_directory(self, path):
        if not _directory_path_valid(path):
            raise Error("Invalid KiKiMR directory path to create: {!r}.", path)

        if self._ydb_client == KIKIMR_CLIENT_YDB:
            try:
                _wrap_ydb_make_directory(self._database_config, path, self._scheme_operation_timeout,
                                         self._database_config.warning_request_time,
                                         _connection_retry_timeout=self._retry_timeout)
            except ydb.Error as ex:
                raise _ydb_convert_exception(ex, "Make directory: '{}'".format(path), self._database_config)
        else:
            query = "SchemeOperation"
            working_dir, name = path.rsplit("/", 1)

            request = TSchemeOperation()
            request.Transaction.ModifyScheme.OperationType = ESchemeOpMkDir
            request.Transaction.ModifyScheme.WorkingDir = working_dir
            request.Transaction.ModifyScheme.MkDir.Name = name

            response = self._send_request("SchemeOperation", request)

            if response.Status == _TResponseStatus.IN_PROGRESS:
                request = TSchemeOperationStatus()
                request.FlatTxId.TxId = response.FlatTxId.TxId
                request.FlatTxId.SchemeShardTabletId = response.FlatTxId.SchemeShardTabletId
                request.PollOptions.Timeout = max(1, self._database_config.scheme_operation_timeout - 1) * 1000

                response = self._send_request("SchemeOperationStatus", request, idempotent=True)

            if response.Status == _TResponseStatus.IN_PROGRESS:
                raise QueryError(query, "Directory creation operation has timed out.")
            elif response.Status != _TResponseStatus.OK:
                raise QueryError(query, _get_error(response.ErrorReason)[1])

    def delete_directory(self, path):
        if not _directory_path_valid(path):
            raise Error("Invalid KiKiMR directory path to delete: {!r}.", path)

        if self._ydb_client == KIKIMR_CLIENT_YDB:
            try:
                _wrap_ydb_remove_directory(self._database_config, path, self._scheme_operation_timeout,
                                           self._database_config.warning_request_time,
                                           _connection_retry_timeout=self._retry_timeout)
            except ydb.Error as ex:
                raise _ydb_convert_exception(ex, "Remove directory '{}'".format(path), self._database_config)
        else:
            query = "SchemeOperation"
            working_dir, name = path.rsplit("/", 1)

            request = TSchemeOperation()
            request.Transaction.ModifyScheme.OperationType = ESchemeOpRmDir
            request.Transaction.ModifyScheme.WorkingDir = working_dir
            request.Transaction.ModifyScheme.Drop.Name = name

            response = self._send_request("SchemeOperation", request)

            if response.Status == _TResponseStatus.IN_PROGRESS:
                request = TSchemeOperationStatus()
                request.FlatTxId.TxId = response.FlatTxId.TxId
                request.FlatTxId.SchemeShardTabletId = response.FlatTxId.SchemeShardTabletId
                request.PollOptions.Timeout = max(1, self._database_config.scheme_operation_timeout - 1) * 1000

                response = self._send_request("SchemeOperationStatus", request, idempotent=True)

            if response.Status == _TResponseStatus.IN_PROGRESS:
                raise QueryError(query, "Directory deletion operation has timed out.")
            elif response.Status != _TResponseStatus.OK:
                error_code, error_message = _get_error(response.ErrorReason)

                if error_code == _KikimrErrorCodes.DIRECTORY_IS_NOT_EMPTY:
                    raise DirectoryIsNotEmptyError(query, error_message)
                else:
                    raise QueryError(query, error_message)

    def copy_table(self, src_path, dst_path, overwrite=False):
        working_dir, dst_table = dst_path.rsplit("/", 1)
        if not _directory_path_valid(working_dir):
            raise Error("Invalid KiKiMR directory path to work on: {!r}.", working_dir)

        if overwrite:
            try:
                self.query("DROP TABLE [{}]".format(dst_path), transactionless=True)
            except TableNotFoundError:
                pass

        if self._ydb_client == KIKIMR_CLIENT_YDB:
            with _ydb_session_context(self._database_config) as session:
                try:
                    _wrap_ydb_copy_table(session, src_path, dst_path, self._scheme_operation_timeout,
                                         self._database_config.warning_request_time,
                                         _connection_retry_timeout=self._retry_timeout)
                except ydb.Error as ex:
                    raise _ydb_convert_exception(ex, "copy_table from '{}' to '{}'".format(src_path, dst_path),
                                                 self._database_config)
        else:
            query = "SchemeOperation"

            request = TSchemeOperation()
            request.Transaction.ModifyScheme.OperationType = ESchemeOpCreateTable
            request.Transaction.ModifyScheme.WorkingDir = working_dir
            request.Transaction.ModifyScheme.CreateTable.Name = dst_table
            request.Transaction.ModifyScheme.CreateTable.CopyFromTable = src_path

            response = self._send_request("SchemeOperation", request)

            if response.Status == _TResponseStatus.IN_PROGRESS:
                request = TSchemeOperationStatus()
                request.FlatTxId.TxId = response.FlatTxId.TxId
                request.FlatTxId.SchemeShardTabletId = response.FlatTxId.SchemeShardTabletId
                request.PollOptions.Timeout = max(1, self._database_config.scheme_operation_timeout - 1) * 1000

                response = self._send_request("SchemeOperationStatus", request, idempotent=True)

            if response.Status == _TResponseStatus.IN_PROGRESS:
                raise QueryError(query, "Copy table operation has timed out.")
            elif response.Status != _TResponseStatus.OK:
                error_code, error_message = _get_error(response.ErrorReason)
                raise QueryError(query, error_message)

    def _prepare_query(self, query: [str, QueryTemplate]) -> _PreparedQuery:
        """
        WARNING: Can change/remove in near future. Use for tests only.

        Prepare YDB query. Now all variables for prepared statements must start with $ydb_ for exclude intersect with
        internal variables (for example - table name vars).

        Prepared statements can be used only with this session. Session is this connection and related connections
        (for example transactions, created with transaction() method).

        :return: Object with prepared statement.
        """
        query_template = query if isinstance(query, QueryTemplate) else sql.build_query_template(query, variables=self._scope.table_names)
        text_for_prepare = query_template.build_text_for_prepare()
        if self._ydb_session_val is None:
            raise LogicalError("Can prepare query with session only")
        if self._ydb_session_val.has_prepared(text_for_prepare):
            metrics.query_prepared_cache_counter.labels(metrics.QueryPreparedCounterCache.CACHED).inc()
        else:
            metrics.query_prepared_cache_counter.labels(metrics.QueryPreparedCounterCache.COMPILE).inc()

        prepared_query = _wrap_ydb_prepare_query(self._ydb_session_val, text_for_prepare, timeout=self._database_config.ydb_prepare_timeout,
                                                 _connection_retry_timeout=self._retry_timeout)
        return _PreparedQuery(self._ydb_session_val.session_id, text_for_prepare, prepared_query)

    def select(self, query, *args, model: Type[_TAny]=None, ignore_unknown=False, validate=True,
               strip_table_from_columns=None, partial=False, single_shot_only=False, commit=False,
               expected_result_count=1, force_prepare_mode: bool=None) -> Iterable[Union[_TAny, dict]]:
        """Attention: By default all SELECT's are processed without a COMMIT, so may return an inconsistent data.
        Model and strip_columns will apply for result of first query only (if was send several results)"""

        if expected_result_count < 1:
            LogicalError("Select query return minimum one result.")

        if strip_table_from_columns is None and model is not None:
            strip_table_from_columns = ColumnStrippingStrategy.STRIP

        response = self._query(query, *args, single_shot_only=single_shot_only, read_only=True, commit=commit,
                               force_prepare_mode=force_prepare_mode)

        try:
            if self._ydb_client == KIKIMR_CLIENT_YDB:
                results = _parse_select_response_ydb(response, expected_result_count)
            else:
                results = _parse_select_response(response, expected_result_count)

            # Apply model and stip_table for first result only
            result = results[0]

            if strip_table_from_columns or model is not None:
                for row_index, row in enumerate(result):
                    if strip_table_from_columns:
                        strip_table_name_from_row(row, strip_table_from_columns)
                    if model is not None:
                        result[row_index] = model.from_kikimr(row, ignore_unknown=ignore_unknown, validate=validate, partial=partial)

            if expected_result_count == 1:
                return results[0]
            else:
                return results
        except Exception as e:
            raise QueryError(query, "{}", e)

    def select_paged_subquery(self, query, *args, limit=None, max_select_rows=None, model: Type[_TAny]=None, ignore_unknown=False,
                              strip_table_from_columns=None, partial=False, validate=True,
                              commit=False) -> Iterable[Union[_TAny, dict]]:
        """
        Use as select paged with iteration by subquery. One of args item must be SqlCursorSubquery.
        SqlCursorCondition, SqlCursorOrderLimit must be in SqlCursorSubquery args only.

        :param max_select_rows: max rows, selected per page
        :param limit: limit of total returned rows.

        Remaining arguments similar to select_paged method.
        """

        if max_select_rows is None:
            max_select_rows = self._database_config.max_select_rows

        cursor_query_spec = None  # type: sql.SqlCursorSubquery
        for arg in args:
            if isinstance(arg, sql.SqlCursorSubquery):
                if cursor_query_spec is not None:
                    raise LogicalError("Invalid cursor query specification: Double subquery arguments found.")
                cursor_query_spec = arg

        if cursor_query_spec is None:
            raise LogicalError("Invalid cursor query specification: No subquery argument.")

        subquery_count = 0

        wrap_query_args = []
        cursor_query, cursor_query_args = cursor_query_spec.query.render()
        wrap_query = "{} = ({}); ".format(cursor_query_spec.variable_name, cursor_query)
        wrap_query_args.extend(cursor_query_args)

        wrap_query += query
        wrap_query_args.extend(args)
        if limit is not None:
            wrap_query += " ?"
            wrap_query_args.append(SqlLimit(limit))
        wrap_query += "; "
        result_index_main = subquery_count
        subquery_count += 1

        wrap_query += "SELECT COUNT(*) as cnt FROM ?; "
        wrap_query_args.append(cursor_query_spec)
        result_index_subquery_rows_count = subquery_count
        subquery_count += 1

        wrap_query += "SELECT * FROM ? ? LIMIT 1; "
        wrap_query_args.append(cursor_query_spec)
        wrap_query_args.append(cursor_query_spec.cursor_spec.get_sql_order().reverse())
        result_index_last_subquery_line = subquery_count
        subquery_count += 1

        class Pager(_SelectPager):
            def __init__(self):
                super().__init__(max_select_rows)
                self.__returned_rows = 0

            def get_rows_from_page(self, results):
                res = results[result_index_main]
                if limit is not None:
                    remain_limit = limit - self.__returned_rows
                    if len(res) > remain_limit:
                        res = res[:remain_limit]
                self.__returned_rows += len(res)
                return res

            def is_last_page(self, results):
                count_result = results[result_index_subquery_rows_count]
                read_all_rows_from_cursor_table = int(count_result[0]["cnt"]) < self.max_select_rows
                reached_total_limit = self.__returned_rows >= limit if limit is not None else False
                return read_all_rows_from_cursor_table or reached_total_limit

            @classmethod
            def get_cursor_values(cls, results, render_column, cursor_columns):
                cursor_results = results[result_index_last_subquery_line]
                row = cursor_results[0]
                return cls._get_cursor_values_from_row(row, strip_table_from_columns, model, render_column,
                                                       cursor_columns)

        pager = Pager()

        return self._select_paged_worker(cursor_query_spec.cursor_spec, wrap_query,
                                         pager, *wrap_query_args, model=model,
                                         ignore_unknown=ignore_unknown,
                                         strip_table_from_columns=strip_table_from_columns, partial=partial,
                                         validate=validate, commit=commit, expected_kikimr_result_count=subquery_count)

    def select_paged(self, cursor_spec: KikimrCursorSpec, query, *args, max_select_rows=None, model: Type[_TAny]=None, ignore_unknown=False,
                     strip_table_from_columns=None, partial=False, validate=True, commit=False) -> Iterable[Union[_TAny, dict]]:

        if max_select_rows is None:
            max_select_rows = self._database_config.max_select_rows

        class Pager(_SelectPager):
            def __init__(self):
                super().__init__(max_select_rows)

            @classmethod
            def get_rows_from_page(cls, results):
                return results

            def is_last_page(self, results):
                return len(results) < self.max_select_rows

            @classmethod
            def get_cursor_values(cls, results, render_column, cursor_columns):
                row = results[len(results)-1]
                return cls._get_cursor_values_from_row(row, strip_table_from_columns, model, render_column,
                                                       cursor_columns)

        pager = Pager()

        return self._select_paged_worker(cursor_spec, query, pager, *args, model=model, ignore_unknown=ignore_unknown,
                                         strip_table_from_columns=strip_table_from_columns, partial=partial, validate=validate, commit=commit)

    def _select_paged_worker(self, cursor_spec: KikimrCursorSpec, query, pager: _SelectPager, *args,
                             model: Type[_TAny]=None, ignore_unknown=False, strip_table_from_columns=None, partial=False,
                             validate=True, commit=False, expected_kikimr_result_count=1) -> Iterable[Union[_TAny, dict]]:
        """
        Attention:
        * By default all SELECT's are processed without a COMMIT, so may return an inconsistent data.
        * commit=True sends a COMMIT after each page query - not after all page queries, so can't be used inside of
          transaction.
        """

        if cursor_spec.table_alias is None:
            def render_column(column_name):
                return column_name
        else:
            def render_column(column_name):
                return cursor_spec.table_alias + "." + column_name

        args = list(args)

        where_condition_index = None
        where_condition = None
        order_limit_index = None

        for index, arg in enumerate(args):
            if isinstance(arg, sql.SqlCursorCondition):
                if where_condition_index is not None:
                    raise LogicalError("Invalid paged query specification.")

                where_condition_index = index
                where_condition = arg
            elif isinstance(arg, sql.SqlCursorOrderLimit):
                if order_limit_index is not None:
                    raise LogicalError("Invalid paged query specification.")

                order_limit_index = index

        if where_condition_index is None or order_limit_index is None or order_limit_index <= where_condition_index:
            raise LogicalError("Invalid paged query specification.")

        primary_key_columns = cursor_spec.table.initial_spec().primary_keys
        if not primary_key_columns:
            raise LogicalError()

        cursor_columns = primary_key_columns[:]

        for column, value in where_condition.fixed_primary_key_values.items():
            try:
                cursor_columns.remove(column)
            except ValueError:
                raise LogicalError("Invalid paged query specification: Primary key doesn't contain {!r} column.",
                                   column)

        cursor_columns_desc = cursor_columns[:] if cursor_spec.reverse_order else None

        cursor_values = None

        while True:
            where_condition_query = sql.SqlCondition()

            for column, value in where_condition.fixed_primary_key_values.items():
                where_condition_query.and_condition(render_column(column) + " = ?", value)

            if cursor_values is not None:
                where_condition_query.and_condition(
                    sql.SqlCompoundKeyCursor(list(map(render_column, cursor_columns)), cursor_values, keys_desc=cursor_columns_desc))

            args[where_condition_index] = where_condition_query
            args[order_limit_index] = sql.SqlCompoundKeyOrderLimit(
                list(map(render_column, primary_key_columns)), pager.max_select_rows, keys_desc=cursor_columns_desc)

            select_result = self.select(
                query, *args, single_shot_only=commit, model=model, strip_table_from_columns=strip_table_from_columns,
                ignore_unknown=ignore_unknown, partial=partial, validate=validate, commit=commit,
                expected_result_count=expected_kikimr_result_count
            )

            yield from pager.get_rows_from_page(select_result)

            if pager.is_last_page(select_result):
                break

            cursor_values = pager.get_cursor_values(select_result, render_column, cursor_columns)

    def select_one(self, query, *args, model: Type[_TAny]=None, strip_table_from_columns=None, ignore_unknown=False,
                   commit=False) -> Union[_TAny, dict]:
        """Attention: By default all SELECT's are processed without a COMMIT, so may return an inconsistent data."""

        result = None

        for index, result in enumerate(self.select(
            query, *args, model=model, strip_table_from_columns=strip_table_from_columns, ignore_unknown=ignore_unknown,
            commit=commit
        )):
            if index == 0:
                pass
            else:
                raise QueryError(query, "Got several rows when expected only one.")

        return result

    def __get_values_from_dict(self, data: dict, table: KikimrTable, columns: Sequence[str]) -> List[Union[Variable, Any]]:
        if columns is None:
            columns = data.keys()

        if table is None and self._scope.default_table is not None:
            table = self._scope.default_table

        if table is None:
            values = [data[key] for key in columns]
        else:
            values = [table.var(key, data[key]) for key in columns]

        return values

    def insert_object(self, query, obj, commit=None, enable_batching=None, table: KikimrTable=None):
        if isinstance(obj, Model):
            obj.copy().validate()  # Model.validate() modifies the object, so we have to make a copy
            data = obj.to_kikimr()
        else:
            data = obj

        if not data:
            raise ValidationError("An attempt to insert an empty object.")

        columns = data.keys()
        query += " ({columns}) VALUES ({values})".format(
            columns=", ".join(columns), values=", ".join([sql.VALUE_PLACEHOLDER] * len(data)))

        values = self.__get_values_from_dict(data, table, columns=columns)

        self.query(query, *values, commit=commit, enable_batching=enable_batching)

    def update_object(self, query, obj, *args, commit=None, enable_batching=None, table: KikimrTable=None):
        if isinstance(obj, Model):
            obj.copy().validate(partial=True)  # Model.validate() modifies the object, so we have to make a copy
            data = obj.to_kikimr()
        else:
            data = obj

        if not data:
            raise ValidationError("An attempt to update with an empty object.")

        orig_query = query
        columns = data.keys()  # need for preserver columns order
        query = sql.substitute_variable(query, "set", "SET " + ", ".join(
            column_name + " = ?" for column_name in columns))

        if query == orig_query:
            raise QueryError(query, "Update query must include $set variable.")

        values = self.__get_values_from_dict(data, table, columns=columns)

        self.query(query, *(tuple(values) + args), commit=commit, enable_batching=enable_batching)

    def update_on(self, query: str, columns: List[str], *objects: List[Union[Model, dict]], commit=None, enable_batching=None, table: KikimrTable=None):
        if not columns:
            raise ValidationError("An attempt to perform 'update on' without columns.")

        if not objects:
            raise ValidationError("An attempt to perform 'update on' without objects.")

        def _to_kikimr(obj) -> dict:
            if isinstance(obj, Model):
                obj.copy().validate(partial=True)  # Model.validate() modifies the object, so we have to make a copy
                data = obj.to_kikimr()
            else:
                data = obj

            res = {key: data[key] for key in columns}
            return res

        rows_count = 0
        values = []
        for data in (_to_kikimr(obj) for obj in objects):
            values.extend(self.__get_values_from_dict(data, table, columns))
            rows_count += 1

        placeholders = "({value_placeholders})".format(value_placeholders=", ".join([sql.VALUE_PLACEHOLDER] * len(columns)))

        query += " ({columns}) VALUES {values}".format(
            columns=", ".join(columns), values=", ".join([placeholders] * rows_count))

        self.query(query, *values, commit=commit, enable_batching=enable_batching, force_prepare_mode=False)

    def query(self, query, *args, transactionless=False,
              idempotent=False, commit=None, enable_batching=None, force_prepare_mode: bool=None):
        """Expected to be used for all non-SELECT queries,
        which don't return values and may be optionally batched.
        """
        if isinstance(query, _PreparedQuery):
            if len(args) > 0 and not isinstance(args[0], dict):
                raise LogicalError("Prepared query arg must be variables dictionary.")
            if len(args) > 1:
                raise LogicalError("Prepared query can has maximum one argument.")

        if type(query) in (tuple, list):
            if args:
                raise LogicalError()
            else:
                args = tuple(itertools.chain.from_iterable(sub_query[1:] for sub_query in query))

            query = "; ".join(sub_query[0] for sub_query in query)

        self._query(query, *args, transactionless=transactionless,
                    idempotent=idempotent, commit=commit,
                    enable_batching=enable_batching, force_prepare_mode=force_prepare_mode)

    def _query(self, query, *args, transactionless=False, single_shot_only=False, read_only=False,
               idempotent=False, commit=None, enable_batching=None, force_prepare_mode: bool=None):
        if type(query) in (tuple, list):
            if args:
                raise LogicalError()
            else:
                args = tuple(itertools.chain.from_iterable(sub_query[1:] for sub_query in query))

            query = "; ".join(sub_query[0] for sub_query in query)

        query_options = self._query_options(transactionless=transactionless, single_shot_only=single_shot_only,
                                            read_only=read_only, idempotent=idempotent,
                                            commit=commit, enable_batching=enable_batching,
                                            force_prepare_mode=force_prepare_mode)

        query_func = self._execute_query
        if query_options.pop("auto_retry"):
            query_func = retry_idempotent_kikimr_errors(query_func)

        return query_func(query, *args, **query_options)

    def _execute_query(self, query, *args, read_only, transactionless, idempotent, commit, keep_session,
                       enable_batching, force_prepare_mode):

        def is_transaction_finish_cmd():
            return query == self._COMMIT_COMMAND or query == self._ROLLBACK_COMMAND

        batch = self._get_batch()
        if query == self._COMMIT_COMMAND:
            commit = True

        # HACK: PRAGMAs must never be batched
        # E.g. ReadCommitted must be the first clause in transaction.
        # prepared_query don't batch because it isn't tested with batch
        if isinstance(query, _PreparedQuery) or "pragma" in query.lower():
            enable_batching = False

        # Read-only queries always pass without batching or flushing
        # We flush on commit or not enable_batching.
        if not read_only:
            # The scope can be changed until batch is flushed
            query = sql.substitute_variables(query, self._scope.table_names)

            if not is_transaction_finish_cmd():
                batch.query(query, *args)
            if enable_batching and not is_transaction_finish_cmd():
                return

            # Non-batched queries always flush the batch
            # But rollback:
            # 1. Doesn't need send command to server, then rollback
            # 2. For rollback query must by exact self._ROLLBACK_COMMAND
            if query == self._ROLLBACK_COMMAND:
                batch.consume()  # flush internal buffer
            else:
                query, args = batch.consume()

        elif (commit or is_transaction_finish_cmd()) and not batch.is_empty():
            # Flush batched requests before commit
            if query == self._COMMIT_COMMAND:
                query, args = batch.consume()
            elif query == self._ROLLBACK_COMMAND:
                pass  # Doesn't need real flush commands to database if need rollback transactions
            else:
                q, a = batch.consume()
                query += "; " + q
                args += a

            commit = True
            read_only = False

        query_options = self._query_pre_handler(query, read_only=read_only, commit=commit, keep_session=keep_session)
        query_options["force_prepare_mode"] = force_prepare_mode

        try:
            query, response = self._yql_query(query, *args, read_only_query=read_only, transactionless=transactionless,
                                              idempotent=idempotent, **query_options)
        except Exception as ex:
            self._query_error_handler(query, ex)
            raise

        self._query_post_handler(
            query, request_session_id=query_options.get("session_id"),
            read_only_session=query_options["read_only_session"], keep_session=keep_session, response=response)

        return response

    @__ydb_session_maker
    def _yql_query(self, query, *args, read_only_query, read_only_session,
                   transactionless, idempotent, commit, keep_session, session_id=None, transaction=None,
                   force_prepare_mode: bool):
        if transactionless:
            if session_id is not None or commit or transaction is not None:
                raise ValidationError("An attempt to execute a transactionless query inside of transaction.")

            if keep_session:
                log.error("Executing transactionless `%s` query without session close.", query)

        if query in (self._COMMIT_COMMAND, self._ROLLBACK_COMMAND):
            # COMMIT/ROLLBACK send as special commands/flag. It can't be prepared.
            force_prepare_mode = False

        if query == self._COMMIT_COMMAND and self._ydb_client != KIKIMR_CLIENT_YDB:
            # Treat COMMIT command as a special case here, to activate our COMMIT retry logic
            # For own kikimr client only
            commit = True
            response = None
        else:
            if not keep_session and not read_only_session and not transactionless and not commit:
                raise ValidationError("An attempt to execute a non-read-only, non transactionless query without a COMMIT.")

            try:
                if not isinstance(query, _PreparedQuery):
                    query_template = sql.build_query_template(query, *args, variables=self._scope.table_names)

                    if self._ydb_client != KIKIMR_CLIENT_YDB:
                        auto_prepare = False  # Old driver can't prepare queries.
                    elif force_prepare_mode is not None:
                        auto_prepare = force_prepare_mode
                    elif self._ydb_client != KIKIMR_CLIENT_YDB or transactionless or query_template.is_text_mode_preferred():
                        auto_prepare = False
                    elif query_template.know_all_types:
                        auto_prepare = self._database_config.allow_autoprepare
                    else:
                        auto_prepare = False
                    if auto_prepare:
                        try:
                            query = self._prepare_query(query_template)
                        except ydb.Error as ex:
                            ex = _ydb_convert_exception(ex, query_template.build_text_for_prepare(), self._database_config)
                            if isinstance(ex, CompileQueryError):
                                # fallback to text query
                                query = query_template.build_text_query()
                                log.warning("Error compile ydb query because type conversion. Query: `%s`. Error: `%s`. Fallback to text query.", ex.query, ex.message)
                            else:
                                raise ex
                        else:
                            args = (query_template.values,)
                    else:
                        query = query_template.build_text_query()
            except ValidationError as e:
                raise QueryError(query, "{}", e)

            append_commit = False
            if session_id is None and transaction is None:
                if query not in [self._COMMIT_COMMAND, self._ROLLBACK_COMMAND] and not isinstance(query, _PreparedQuery):
                    table_prefix = self._database_config.get_root_path()
                    query = "PRAGMA TablePathPrefix({}); ".format(sql.render_value(table_prefix)) + query

                if read_only_session or not transactionless:
                    # If this is a first query in the session, it's idempotent and we can safely retry all connection errors.
                    idempotent = True

                    # Even more, for read-only queries we can eliminate extra round trip to the server by inserting COMMIT
                    # command right into the query.
                    # python ydb client need to explicit commit by method/flag, not in query string
                    if read_only_session and commit:
                        commit = False
                        if self._ydb_client == _KIKIMR_CLIENT_YC:
                            query += "; COMMIT"
                        else:
                            append_commit = True

            keep_query_session = keep_session or commit

            if commit and self._ydb_client == KIKIMR_CLIENT_YDB:
                append_commit = True

            query_type = metrics.QueryTypes.SELECTS if read_only_query else metrics.QueryTypes.UPDATES
            response = self._yql_request(query, *args, query_type=query_type, session_id=session_id,
                                         keep_session=keep_query_session, idempotent=idempotent,
                                         retryable=not transactionless, append_commit=append_commit,
                                         transactionless=transactionless, transaction=transaction,
                                         read_only_query=read_only_query)

            if keep_query_session:
                if self._ydb_client != KIKIMR_CLIENT_YDB:
                    session_id = _get_session_id_from_response(query, response, session_id)
            else:
                session_id = None

        if commit and self._ydb_client != KIKIMR_CLIENT_YDB:
            # Commit retries don't work with new client.
            # Now commit apply in query - for eliminate extra round trip to kikimr
            # Connections will be better, because YDB client connect direct to kikimr without load balancers.
            if session_id is None:
                raise LogicalError()

            full_query = query if query == self._COMMIT_COMMAND else query + "; " + self._COMMIT_COMMAND
            full_query = self._get_full_query(full_query)

            # Always keep session on COMMIT to make it idempotent (and thus retryable)
            query_type = metrics.QueryTypes.READ_ONLY_COMMIT if read_only_session else metrics.QueryTypes.READ_WRITE_COMMIT
            self._yql_request(self._COMMIT_COMMAND, full_query=full_query, session_id=session_id, keep_session=True,
                              idempotent=True, query_type=query_type)

            if not keep_session:
                # And only when we successfully committed the transaction, close the session

                try:
                    self._yql_request(self._ROLLBACK_COMMAND, full_query=full_query + "; " + self._ROLLBACK_COMMAND, session_id=session_id,
                                      keep_session=False, query_type=metrics.QueryTypes.ROLLBACK)
                except Exception as e:
                    log.warning("Failed to close %r session: %s", session_id, e)

        return query, response

    def __ydb_request(self, query: Union[str, _PreparedQuery], *args, full_query=None, idempotent=False, retryable=False,
                      append_commit=False, scheme_query=False, transaction: ydb.table.TxContext=None,
                      read_only_query=False
                      ) -> _YdbResponse:
        if self._ydb_session_val is None:
            raise LogicalError("Can exec yql request with existed session only")

        query_text = query._text if isinstance(query, _PreparedQuery) else query
        detailed_query = query_text if full_query is None else "{} as part of {}".format(query_text, full_query)

        commit_only = query == self._COMMIT_COMMAND
        commit = commit_only or append_commit
        rollback = query == self._ROLLBACK_COMMAND
        execute_query = not (commit_only or rollback)

        if commit and rollback:
            raise LogicalError()

        if scheme_query and (commit or rollback):
            raise LogicalError("Commit or rollback in scheme (transactionless) query")

        if scheme_query and transaction is not None:
            raise LogicalError("Transactionless (scheme) query within transaction")

        prepared_args = None
        if isinstance(query, _PreparedQuery):
            if query._session_id != self._ydb_session_val.session_id:
                raise LogicalError("Query `{}` was prepared in other session: {} != {}", query._text, query._session_id,
                                   self._ydb_session_val.session_id)
            if len(args) == 0:
                pass
            if len(args) == 1:
                if isinstance(args[0], dict):
                    prepared_args = args[0]
                else:
                    raise LogicalError("Argument of prepared_query must be dict.")

            if len(args) > 1:
                raise LogicalError("Prepared query can have max 1 argument - variables dict.")
        try:
            if transaction is None and not scheme_query:
                if read_only_query:
                    if append_commit:
                        transaction_mode = _ydb_convert_transaction_mode(TransactionMode.ONLINE_READ_ONLY_CONSISTENT)
                    else:
                        transaction_mode = _ydb_convert_transaction_mode(TransactionMode.ONLINE_READ_ONLY_INCONSISTENT)
                else:
                    transaction_mode = _ydb_convert_transaction_mode(TransactionMode.SERIALIZABLE_READ_WRITE)
                transaction = self._ydb_session_val.transaction(tx_mode=transaction_mode)

                # One time transaction must be commit (or leak). Consistent/inconsistent mode set with transaction mode
                append_commit = True

            if self._autocommit:
                append_commit = True

            result = _YdbResponse()

            if append_commit:
                detailed_query += "; " + self._COMMIT_COMMAND

            if self._database_config.enable_logging:
                log.debug("Executing query with ydb client: `%s`", detailed_query)

            if execute_query:
                request_type = metrics.QueryTypes.SELECTS if read_only_query else metrics.QueryTypes.UPDATES
                try:
                    result.results = self.__ydb_execute_query(transaction, query, detailed_query, idempotent, retryable,
                                                          append_commit, scheme_query, prepared_args, request_type)
                except ydb.Error as ex:
                    raise _ydb_convert_exception(ex, detailed_query, self._database_config)

            if commit_only:
                _wrap_ydb_commit(self._ydb_session_val, transaction, timeout=self.__session_timeout,
                                 log_warning_time=self._database_config.warning_request_time,
                                 readonly_commit=read_only_query, _connection_retry_timeout=self._retry_timeout)
            if rollback:
                _wrap_ydb_rollback(self._ydb_session_val, transaction, timeout=self.__session_timeout,
                                   log_warning_time=self._database_config.warning_request_time,
                                    _connection_retry_timeout=self._retry_timeout)

            return result
        except ydb.Error as ex:
            raise _ydb_convert_exception(ex, detailed_query, self._database_config)

    def __ydb_execute_query(self, transaction: ydb.table.TxContext, query: Union[str, _PreparedQuery], detailed_query,
                            idempotent, retryable, append_commit, scheme_query, prepared_args, request_type):
        request_timeout = self._scheme_operation_timeout if scheme_query else self._request_timeout
        retry_timeout = max(self._retry_timeout, request_timeout)
        current_request_timeout = request_timeout

        if self._database_config.enable_logging:
            log.debug("Ydb executing `%s`:\nTransaction id: %s", detailed_query, transaction.tx_id)

        try:
            if isinstance(query, _PreparedQuery):
                return _wrap_ydb_execute(self._ydb_session_val, transaction, query._prepared,
                    detailed_query=detailed_query, append_commit=append_commit, parameters=prepared_args,
                    timeout=current_request_timeout, log_warning_time=self._database_config.warning_request_time,
                    request_type=request_type, _connection_retry_timeout=retry_timeout)
            elif scheme_query:
                return _wrap_ydb_execute_scheme(self._ydb_session_val, query, timeout=current_request_timeout,
                                                log_warning_time=self._database_config.warning_request_time,
                                                _connection_retry_timeout=retry_timeout)
            else:
                return _wrap_ydb_execute(self._ydb_session_val, transaction, query, detailed_query=detailed_query,
                                         append_commit=append_commit, timeout=current_request_timeout,
                                         log_warning_time=self._database_config.warning_request_time,
                                         _connection_retry_timeout=retry_timeout)
        except (ydb.ConnectionError, ydb.Timeout) as e:
            timed_out = isinstance(e, ydb.Timeout)
            if retryable:
                raise (RetryableRequestTimeoutError if timed_out else RetryableConnectionError)(
                    self._database_config, query, "{}", e,
                    request_timeout=request_timeout, retry_timeout=retry_timeout)
            else:
                raise UnretryableConnectionError(query, "{}", e)

    # FIXME: Support TraceId (https://st.yandex-team.ru/CLOUD-3221)
    def _yql_request(self, query, *args, keep_session=None, query_type=None, full_query=None,
                     session_id=None, idempotent=False, retryable=False, append_commit=False,
                     transactionless=False, transaction: ydb.table.TxContext=None, read_only_query=False) \
            -> Union[TQueryResponse, _YdbResponse]:
        if self._ydb_client == KIKIMR_CLIENT_YDB:
            return self.__ydb_request(query, *args, full_query=full_query, idempotent=idempotent, retryable=retryable,
                                      append_commit=append_commit, scheme_query=transactionless, transaction=transaction,
                                      read_only_query=read_only_query)

        request = TYqlRequest()
        request.RequestType = kqp_pb2.REQUEST_TYPE_PROCESS_QUERY

        kikimr_query = request.QueryRequest
        if session_id is not None:
            kikimr_query.SessionId = session_id
        kikimr_query.Type = kqp_pb2.QUERY_TYPE_SQL
        kikimr_query.Query = query
        kikimr_query.KeepSession = keep_session

        detailed_query = query if full_query is None else "{} as part of {}".format(query, full_query)

        response = self._send_request(
            "YqlRequest", request, query=detailed_query, query_type=query_type, idempotent=idempotent, retryable=retryable)

        if response.KqpStatus == kqp_pb2.STATUS_SUCCESS:
            errors, warnings = _parse_query_issues(response.QueryResponse.QueryIssues)
            _log_query_issues(detailed_query, "error", errors)
            _log_query_issues(detailed_query, "warning", warnings)
            return response

        _handle_error(detailed_query, response, self._database_config)

    def _send_request(self, method, request, query=None, idempotent=False, retryable=False, query_type=metrics.QueryTypes.OTHER):
        """
        :param idempotent: The request is idempotent and thus may be safely retried as is.
        :param retryable: The request may be retried as a part of another session.
        """

        if self._ydb_client == KIKIMR_CLIENT_YDB:
            raise LogicalError("Send request by old kikimr client, while new should.")

        if query is None:
            query = method

        if method == "SchemeOperation":
            request_timeout = self._scheme_operation_timeout
        else:
            request_timeout = self._request_timeout

        provided_retry_timeout = self._database_config.retry_timeout if self._retry_timeout is None else self._retry_timeout
        retry_timeout = max(provided_retry_timeout, request_timeout)
        connection_try_number = 1
        current_request_timeout = request_timeout
        connection_errors_tracking_start_time = time.monotonic()

        pending_query_try_number = 1
        pending_query_errors_start_time = None
        pending_query_retry_timeout = retry_timeout

        while True:
            if self._database_config.enable_logging:
                log.debug("Executing `%s`:\n%s", query, request)

            metrics.query_counter.inc()

            try:
                response = self._grpc_connection(method, request, timeout=current_request_timeout, query=query,
                                                 warning_request_time=self._database_config.warning_request_time,
                                                 warning_period=self._database_config.warning_period, query_type=query_type)
            except grpc.RpcError as e:
                metrics.query_errors_counter.labels(metrics.QueryErrorTypes.CONNECTION).inc()

                try:
                    timed_out = e._state.code == grpc.StatusCode.DEADLINE_EXCEEDED
                    reason = e._state.details.decode("utf8")
                except Exception:
                    timed_out = False
                    reason = str(e)

                if idempotent:
                    error = QueryError(query, "{}", reason)

                    retry_timeout_time = connection_errors_tracking_start_time + retry_timeout
                    sleep_time = 0 if timed_out else 1
                    current_retry_timeout = retry_timeout_time - (time.monotonic() + sleep_time)

                    if current_retry_timeout >= min(1, request_timeout):
                        log.warning("%s (%s) Retrying send request rpc error (#%s)...", error, type(e), connection_try_number)
                        connection_try_number += 1

                        current_request_timeout = min(request_timeout, current_retry_timeout)
                        time.sleep(sleep_time)
                        continue

                    raise error

                if retryable:
                    raise (RetryableRequestTimeoutError if timed_out else RetryableConnectionError)(
                        self._database_config, query, "{}", reason,
                        request_timeout=request_timeout, retry_timeout=retry_timeout)
                else:
                    raise UnretryableConnectionError(query, "{}", reason)
            else:
                connection_try_number = 1
                connection_errors_tracking_start_time = time.monotonic()

            if self._database_config.enable_logging:
                log.debug("Got response on `%s`:\n%s", query, response)

            # At this time only YQL queries return response with KqpStatus
            if getattr(response, "KqpStatus", None) == kqp_pb2.STATUS_BUSY_PENDING_QUERY:
                metrics.query_errors_counter.labels(metrics.QueryErrorTypes.PENDING_QUERY).inc()

                errors, warnings = _parse_query_issues(response.QueryResponse.QueryIssues)
                _log_query_issues(query, "warning", warnings)

                error = QueryError(query, "{}", _get_error_message(errors))

                if pending_query_errors_start_time is None:
                    pending_query_errors_start_time = time.monotonic()

                    # FIXME: A temporary workaround for periodically very slow CREATE TABLE queries
                    if "CREATE TABLE" in query:
                        pending_query_retry_timeout = self._database_config.scheme_operation_timeout

                pending_query_timeout_time = pending_query_errors_start_time + pending_query_retry_timeout

                sleep_time = 1
                expected_request_time = 1

                if pending_query_timeout_time - (time.monotonic() + sleep_time) > expected_request_time:
                    log.warning("%s Retrying (#%s)...", error, pending_query_try_number)
                    pending_query_try_number += 1
                    time.sleep(sleep_time)
                    continue

                raise error

            return response

    @abc.abstractmethod
    def _query_pre_handler(self, query, read_only, commit, keep_session) -> dict:
        pass

    @abc.abstractmethod
    def _query_post_handler(self, query, read_only_session, keep_session, response, request_session_id=None):
        pass

    @abc.abstractmethod
    def _query_error_handler(self, query, exception):
        pass

    @abc.abstractmethod
    def _query_options(self, transactionless, single_shot_only, read_only, idempotent, commit=None,
                       enable_batching=None, force_prepare_mode: bool = None) -> dict:
        pass

    @abc.abstractmethod
    def _get_full_query(self, query):
        pass

    @abc.abstractmethod
    def _get_batch(self) -> sql.SqlBatchModification:
        pass


class _KikimrSimpleConnection(_KikimrBaseConnection):
    """
    It has internal ydb session pool.
    Should use with context manager for prevent sessions leak.
    """
    def transaction(self, tx_mode: TransactionMode = None):
        """
        Transaction should used with context manager for prevent sessions leak.
        """
        return _KikimrTxConnection(self._database_id, self._database_config, self._scope,
                                   request_timeout=self._request_timeout, retry_timeout=self._retry_timeout,
                                   ydb_client=self._ydb_client, tx_mode=tx_mode
                                   )

    def _query_options(self, transactionless, single_shot_only, read_only, idempotent,
                       commit=None, enable_batching=None, force_prepare_mode: bool = None):

        if enable_batching:
            raise ValidationError("An attempt to enable batching for non-transaction query.")

        if transactionless:
            if commit is None:
                commit = False
            elif commit:
                raise ValidationError("An attempt to COMMIT a transactionless command.")

        return dict(
            transactionless=transactionless, read_only=read_only, idempotent=idempotent or read_only,
            commit=True if commit is None else commit,
            keep_session=False, auto_retry=True, enable_batching=False, force_prepare_mode=force_prepare_mode
        )

    def _query_pre_handler(self, query, **kwargs):
        kwargs["read_only_session"] = kwargs.pop("read_only")
        return kwargs

    def _query_post_handler(self, query, **kwargs):
        pass

    def _query_error_handler(self, query, exception):
        pass

    def _get_full_query(self, query):
        return query

    def _get_batch(self):
        return sql.SqlBatchModification()


class _KikimrTxConnection(_KikimrBaseConnection):
    """
    It has internal ydb session pool.
    Should use with context manager for prevent sessions leak.
    """
    def __init__(self, *args, tx_mode: TransactionMode, **kwargs):
        super().__init__(*args, **kwargs)
        self.__tx_start_time = None
        self.__batch = sql.SqlBatchModification()
        self.__transaction_mode = _ydb_convert_transaction_mode(tx_mode)
        if tx_mode in (TransactionMode.ONLINE_READ_ONLY_INCONSISTENT, TransactionMode.ONLINE_READ_ONLY_CONSISTENT,
                       TransactionMode.STALE_READ_ONLY):
            self._autocommit = True
        self.__reset()
        self.__transaction_finished = False

        if self._ydb_client != KIKIMR_CLIENT_YDB:
            if tx_mode == TransactionMode.SERIALIZABLE_READ_WRITE or tx_mode is None:
                pass  # default level
            elif tx_mode == TransactionMode.ONLINE_READ_ONLY_CONSISTENT:
                self.query("""PRAGMA kikimr.IsolationLevel = 'ReadCommitted'""")
            elif tx_mode == TransactionMode.ONLINE_READ_ONLY_INCONSISTENT:
                self.query("""PRAGMA kikimr.IsolationLevel = 'ReadUncommitted'""")
            elif tx_mode == TransactionMode.STALE_READ_ONLY:
                self.query("""PRAGMA kikimr.IsolationLevel = 'ReadStale'""")
            else:
                raise LogicalError("Unknown transaction mode: {}", tx_mode)

    def __finish_transaction(self, exc_val=None):
        self.__transaction_finished = True
        if self._ydb_client == KIKIMR_CLIENT_YDB:
            _ydb_session_put(self._database_config, self._ydb_session_val, exc_val)
            self.__transaction = None
            self._ydb_session_val = None

    def rollback(self, exc_val=None):
        if self.__transaction_finished:
            return

        try:
            if self._ydb_client == KIKIMR_CLIENT_YDB:
                broken = False
                if self.__transaction is not None:
                    try:
                        self._query(self._ROLLBACK_COMMAND, idempotent=True)
                    except QueryError as e:
                        broken = True
                        log.error("Failed to ROLLBACK transaction '%s' session '%s': %s",
                                  self.__transaction.tx_id, self.__transaction.session_id, e)
                self.__reset(broken=broken)
            else:
                session_id = self.__session_id
                self.__reset()

                if session_id is None:
                    return

                try:
                    self._yql_request(self._ROLLBACK_COMMAND, session_id=session_id, keep_session=False,
                                      query_type=metrics.QueryTypes.ROLLBACK)
                except Exception as e:
                    log.error("Failed to ROLLBACK %r session: %s", session_id, e)
        finally:
            self.__finish_transaction(exc_val=exc_val)

    def commit(self):
        if self.__transaction_finished:
            return

        try:
            if self.__session_id is not None or not self.__batch.is_empty() or \
                    self._ydb_client == KIKIMR_CLIENT_YDB and self.__transaction.tx_id is not None:
                self._query(self._COMMIT_COMMAND, read_only=True, transactionless=False, idempotent=True, commit=True)
        finally:
            self.__finish_transaction()

    def __enter__(self):
        super().__enter__()

        self.__ensure_transaction_started()

        return self

    def _tmp_trans_start(self):
        self.__ensure_transaction_started()

    def __ensure_transaction_started(self):
        if self._ydb_client != KIKIMR_CLIENT_YDB:
            return

        if self.__transaction is not None:
            return

        if self._ydb_client == KIKIMR_CLIENT_YDB:
            self._ydb_session_val = _ydb_session_get(self._database_config)
            self.__transaction = self._ydb_session_val.transaction(self.__transaction_mode)  # type: ydb.TxContext

    def __exit__(self, exc_type, exc_val, exc_tb):
        try:
            if exc_val is None:
                self.commit()
            else:
                if self.__broken:
                    log.info("Skip rollback broken transaction.")
                else:
                    self.rollback(exc_val=exc_val)
        finally:
            super().__exit__(exc_type, exc_val, exc_tb)

    def _query_options(self, transactionless, single_shot_only, read_only, idempotent, commit=None,
                       enable_batching=None, force_prepare_mode: bool = None):
        if transactionless:
            raise ValidationError("An attempt to execute a transactionless command inside of transaction.")

        if single_shot_only:
            raise ValidationError("An attempt to execute a single-shot-only command inside of transaction.")

        if enable_batching is None:
            enable_batching = self._database_config.enable_batching

        return dict(
            transactionless=transactionless, read_only=read_only, idempotent=idempotent or read_only,
            commit=False if commit is None else commit,
            keep_session=not commit,
            auto_retry=False,
            enable_batching=enable_batching,
            force_prepare_mode=force_prepare_mode
        )

    def _query_pre_handler(self, query, read_only, commit, keep_session):
        if self.__broken:
            raise QueryError(query, "The transaction is broken.")

        if self.__transaction_finished:
            raise LogicalError("Query in transaction after commit/rollback")

        self.__ensure_transaction_started()

        if self.__tx_start_time is None:
            self.__tx_start_time = time.monotonic()
        session_id = self.__session_id
        read_only_session = self.__read_only and read_only

        if not keep_session:
            self.__session_id, self.__read_only = None, True

        res = dict(read_only_session=read_only_session, commit=commit, keep_session=keep_session)
        if self._ydb_client == KIKIMR_CLIENT_YDB:
            res["transaction"] = self.__transaction
        else:
            res["session_id"] = session_id
        return res

    def _query_post_handler(self, query, read_only_session, keep_session, response: Union[TQueryResponse, _YdbResponse],
                            request_session_id=None):

        if keep_session:
            if self._ydb_client == KIKIMR_CLIENT_YDB:
                response_session_id = None
            else:
                response_session_id = _get_session_id_from_response(query, response, request_session_id)
            self.__session_id, self.__read_only = response_session_id, read_only_session
            self.__queries.append(query)
        else:
            self._query_duration(metrics.TxTypes.COMMITTED)
            self.__queries.clear()

    def _query_error_handler(self, query, exception):
        self.__reset(broken=True, exception=exception)

    def _query_duration(self, tx_type):
        if self.__tx_start_time is not None:
            duration = (time.monotonic() - self.__tx_start_time) * 1000
            metrics.transaction_duration.labels(tx_type).observe(duration)
            self.__tx_start_time = None

    def _get_full_query(self, query):
        return "; ".join(self.__queries + [query])

    def _get_batch(self):
        return self.__batch

    def __reset(self, broken=False, exception=None):
        self.__session_id = None
        self.__read_only = True
        self.__queries = []
        self.__broken = broken
        self._query_duration(metrics.TxTypes.FAILED if broken else metrics.TxTypes.ROLLED_BACK)
        self.__batch.consume()

        if self._ydb_client == KIKIMR_CLIENT_YDB:
            if self._ydb_session_val is None:
                self.__transaction = None
            else:
                if broken:
                    self._ydb_session_val = _ydb_session_renew(self._database_config, self._ydb_session_val, exception)
                self.__transaction = self._ydb_session_val.transaction(tx_mode=self.__transaction_mode)


class _GrpcChannelPool:
    def __init__(self):
        self.__lock = Lock()
        self.__channels = {}

    # FIXME: Ignore old channels which may be forgotten by balancer?
    def get_channel(self, host) -> TGRpcServerStub:
        with self.__lock:
            try:
                channel = self.__channels[host]
            except KeyError:
                options = (("grpc.max_receive_message_length", _MAX_RECEIVE_MESSAGE_LENGTH),)
                channel = grpc.insecure_channel(host, options)
                channel = self.__channels[host] = TGRpcServerStub(channel)

        return channel

    def on_broken_channel(self, host, channel):
        # gRPC detects broken connections and reopens them, but manual reopening is generally faster.

        with self.__lock:
            if self.__channels.get(host) is channel:
                del self.__channels[host]


class _GrpcConnection:
    __channel_pool = _GrpcChannelPool()

    def __init__(self, host):
        self.__host = host

    def __call__(self, method, request, *, timeout, query, warning_request_time, warning_period, query_type):
        """
        Notes:

        gRPC doesn't provide any method to close an opened channel and fully relies on Python GC which looks like a bad
        technical decision, so this class wraps the channel to ensure that it will be collected by reference counter
        instead of GC.

        By default all gRPC methods don't have a timeout and hang forever even if the specified endpoint has an invalid
        DNS name, so some reasonable timeout always should be specified to not accidentally lock the service.
        """

        wait_time = warning_request_time
        channel = self.__channel_pool.get_channel(self.__host)

        start_time = time.monotonic()

        try:
            future = getattr(channel, method).future(request, timeout=timeout)

            while True:
                try:
                    result = future.result(timeout=wait_time)
                except grpc.FutureTimeoutError:
                    log.warning("`%s` query is hanging for %.1f seconds.", query, time.monotonic() - start_time)
                    wait_time = warning_period
                    continue

                return result
        except grpc.RpcError:
            self.__channel_pool.on_broken_channel(self.__host, channel)
            raise
        finally:
            request_time = time.monotonic() - start_time
            metrics.query_duration.labels(method, query_type).observe(request_time * 1000)
            if request_time >= warning_request_time:
                log.warning("`%s` query took %.1f seconds.", query, request_time)

            del channel


def get_kikimr_client(database_id, **kwargs):
    database_config = get_database_config(database_id)
    return _KikimrSimpleConnection(database_id, database_config, **kwargs)


def get_custom_kikimr_client(database_id, host, root, database_name=None, **kwargs):
    database_config = KikimrEndpointConfig()
    database_config.host = host
    database_config.root = root
    database_config.database = database_name
    return _KikimrSimpleConnection(database_id, database_config, **kwargs)


def _get_session_id_from_response(query, response: TQueryResponse, request_session_id=None):
    session_id = response.QueryResponse.SessionId
    if not session_id:
        raise QueryError(query, "{}", _UnexpectedResponseError(details=response))

    if request_session_id is not None and session_id != request_session_id:
        raise QueryError(query, "Got a response with an invalid session ID.")

    return session_id


def _get_error(code):
    try:
        if not code:
            raise ValueError

        if type(code) is bytes:
            code = code.decode("utf-8")

        message = code
        if not message.endswith("."):
            message += "."

        return code, message
    except ValueError:
        return "", "Unknown error."


def _handle_error(query, response: TYqlResponse, database_config: KikimrEndpointConfig):
    if response.QueryResponse.QueryIssues:
        errors, warnings = _parse_query_issues(response.QueryResponse.QueryIssues)
        _log_query_issues(query, "warning", warnings)
        error_message = _get_error_message(errors)
    else:
        error_code, error_message = _get_error(response.KqpError)

    if response.KqpStatus in (kqp_pb2.STATUS_LOCKS_INVALIDATED, kqp_pb2.STATUS_LOCKS_ACQUIRE_FAILURE):
        metrics.query_errors_counter.labels(metrics.QueryErrorTypes.INVALIDATED_LOCKS).inc()
        raise TransactionLocksInvalidatedError(database_config, query, error_message)
    elif response.KqpStatus == kqp_pb2.STATUS_TEMPORARILY_UNAVAILABLE:
        metrics.query_errors_counter.labels(metrics.QueryErrorTypes.INTERNAL).inc()
        raise RetryableError(database_config, query, error_message)
    elif response.KqpStatus == kqp_pb2.STATUS_SCHEME_ERROR:
        metrics.query_errors_counter.labels(metrics.QueryErrorTypes.SCHEME_ERROR).inc()
        raise TableNotFoundError(query, error_message)
    elif response.KqpStatus == kqp_pb2.STATUS_PRECONDITION_FAILED:
        metrics.query_errors_counter.labels(metrics.QueryErrorTypes.PRECONDITION_FAILED).inc()
        raise PreconditionFailedError(query, error_message)
    elif response.KqpStatus == kqp_pb2.STATUS_OVERLOADED:
        metrics.query_errors_counter.labels(metrics.QueryErrorTypes.OVERLOAD).inc()
        raise YdbOverloaded(database_config, query, error_message)
    elif _COLUMN_ALREADY_EXIST_ERROR_RE.search(error_message):
        # FIXME: use KqpStatus (KIKIMR-4835)
        metrics.query_errors_counter.labels(metrics.QueryErrorTypes.SCHEME_ERROR).inc()
        raise ColumnAlreadyExistsError(query, error_message)
    elif _COLUMN_DOES_NOT_EXIST_ERROR_RE.search(error_message):
        # FIXME: use KqpStatus (KIKIMR-4835)
        metrics.query_errors_counter.labels(metrics.QueryErrorTypes.SCHEME_ERROR).inc()
        raise ColumnNotFoundError(query, error_message)
    else:
        log.warning("`%s` query has failed with an unknown KiKiMR error:\n%s", query, response)
        metrics.query_errors_counter.labels(metrics.QueryErrorTypes.UNKNOWN).inc()
        raise QueryError(query, error_message)


def _parse_query_issues(issues):
    errors, warnings = [], []

    for issue in issues:

        if issue.severity <= TSeverityIds.S_ERROR:
            messages = errors
        elif issue.severity == TSeverityIds.S_WARNING:
            messages = warnings
        else:
            continue

        # https://st.yandex-team.ru/CLOUD-23002
        # https://st.yandex-team.ru/CLOUD-25576
        if hasattr(issue, "issues"): # new protobuf
            issues_attr_name = "issues"
        elif hasattr(issue, "issue_message"):
            issues_attr_name = "issue_message"
        else:
            raise LogicalError()

        while getattr(issue, issues_attr_name):
            issue = getattr(issue, issues_attr_name)[-1]

        error_code, error_message = _get_error(issue.message)
        messages.append(error_message)

    return errors, warnings


def _log_query_issues(query, severity_name, messages):
    if not messages:
        return

    log_func = getattr(log, severity_name)

    if len(messages) == 1:
        log_func("`%s` query has produced the following %s: %s", query, severity_name, messages[0])
    else:
        log_func("`%s` query has produced the following %ss:\n%s", query, severity_name, "\n".join(messages))


def _get_error_message(errors):
    if errors:
        return "; ".join(error.rstrip(".") for error in errors) + "."
    else:
        return "Unknown error."


def _parse_select_response_ydb(response: _YdbResponse, expected_result_count) -> List[List]:
    if len(response.results) != expected_result_count:
        raise _UnexpectedResponseCountError(response, expected_result_count, len(response.results))

    result = []
    for res in response.results:
        if res.truncated:
            raise ValidationError("Got a truncated response.")
        else:
            result.append(res.rows)
    return result


def _parse_select_response(response, expected_result_count):
    if len(response.QueryResponse.Results) != expected_result_count:
        raise _UnexpectedResponseCountError(response, expected_result_count, len(response.QueryResponse.Results))

    return[_parse_select_response_one_result(kikimr_result_for_parse, response)
           for kikimr_result_for_parse in response.QueryResponse.Results]


def _parse_select_response_one_result(results, full_response):
    if (
        results.Type.Kind != minikql_pb2.Struct or

        results.Value.WhichOneof("value_value") is not None or
        results.Value.List or results.Value.Tuple or not results.Value.Struct or results.Value.Dict
    ):
        raise _UnexpectedResponseError(details=full_response)

    response_data_types = {member.Name: member_id for member_id, member in enumerate(results.Type.Struct.Member)}
    response_data = results.Value.Struct

    if (
        len(response_data) != len(response_data_types) or
        not set(response_data_types).issuperset({"Data", "Truncated"})
    ):
        raise _UnexpectedResponseError(details=full_response)

    column_types = results.Type.Struct.Member[response_data_types["Data"]].Type
    if column_types.Kind != minikql_pb2.List or column_types.List.Item.Kind != minikql_pb2.Struct:
        raise _UnexpectedResponseError(details=full_response)

    column_names = [column.Name for column in column_types.List.Item.Struct.Member]

    truncated = response_data[response_data_types["Truncated"]]
    if truncated.WhichOneof("value_value") != "Bool":
        raise _UnexpectedResponseError(details=full_response)
    if truncated.Bool:
        raise ValidationError("Got a truncated response.")

    data = response_data[response_data_types["Data"]]

    if (
        data.WhichOneof("value_value") is not None or
        data.Tuple or data.Struct or data.Dict
    ):
        raise _UnexpectedResponseError(details=full_response)

    res = []
    for row in data.List:
        if (
            row.WhichOneof("value_value") is not None or
            row.List or row.Tuple or not row.Struct or row.Dict
        ):
            raise _UnexpectedResponseError(details=full_response)

        if len(row.Struct) != len(column_names):
            raise _UnexpectedResponseError(details=full_response)

        res.append({
            column_name: _get_value(value)
            for column_name, value in safe_zip(column_names, row.Struct)
        })
    return res


def _to_decimal(low_128, hi_128):
    shift = 2 ** 64
    sign_bit = 2 ** 63
    decimal_nan_repr = 10 ** 35 + 1
    decimal_inf_repr = 10 ** 35
    decimal_minus_inf_repr = -(10 ** 35)

    is_negative = (hi_128 & sign_bit) == sign_bit

    hi_128 = (hi_128 - shift) if is_negative else hi_128
    int128_value = low_128 + hi_128 * shift

    if int128_value == decimal_nan_repr:
        return Decimal("Nan")
    elif int128_value == decimal_inf_repr:
        return Decimal("Inf")
    elif int128_value == decimal_minus_inf_repr:
        return Decimal("-Inf")

    return Decimal(int128_value) / Decimal(10 ** DECIMAL_SCALE)


def _get_value(value):
    # TODO: Support all available types

    value_type = value.WhichOneof("value_value")
    if value_type is None:
        # Note: Don't put following checks to the 'top level if' to avoid performance degradation
        # List, Tuple, Dict can be *only* read from Kikimr, e.g. from GROUP BY query
        # You can not store list directly in Kikimr for now.
        if value.List:
            return [_get_value(i) for i in value.List]
        elif value.Tuple:
            return [_get_value(i) for i in value.Tuple]
        elif value.Dict:
            return {_get_value(i.Key): _get_value(i.Payload) for i in value.Dict}
        elif value.Struct:
            pass
        else:
            return None
    elif value_type == "Optional":
        if value.Optional.WhichOneof("value_value") != "Optional":
            return _get_value(value.Optional)
    elif value_type == "Low128":
        # This is Kikimr's notion of Decimals
        low_128, hi_128 = value.Low128, value.Hi128
        return _to_decimal(low_128=low_128, hi_128=hi_128)
    elif value_type in ("Bool", "Int32", "Uint32", "Int64", "Uint64", "Float", "Double", "Bytes", "Text"):
        return getattr(value, value_type)

    log.error("Got a value of an unsupported type from the database: %r.", value)
    raise ValidationError("Got a value of an unsupported type from the database.")


def _directory_path_valid(path: str) -> bool:
    return path.startswith("/") and not path.endswith("/")


# similar to _handle_error for _KIKIMR_CLIENT_YC
# side effect: increment error metrics
def _ydb_convert_exception(ex: ydb.Error, query: str, database_config: KikimrEndpointConfig) -> QueryError:
    if isinstance(ex, QueryError):
        return ex

    error_message = ex.__str__()
    if isinstance(ex, ydb.Aborted):
        metrics.query_errors_counter.labels(metrics.QueryErrorTypes.INVALIDATED_LOCKS).inc()
        return TransactionLocksInvalidatedError(database_config, query, error_message)

    if isinstance(ex, ydb.Unavailable):
        metrics.query_errors_counter.labels(metrics.QueryErrorTypes.INTERNAL).inc()
        return YdbUnavailable(database_config, query, error_message)

    if isinstance(ex, ydb.Overloaded):
        metrics.query_errors_counter.labels(metrics.QueryErrorTypes.OVERLOAD).inc()
        return YdbOverloaded(database_config, query, error_message)

    if isinstance(ex, ydb.SchemeError):
        metrics.query_errors_counter.labels(metrics.QueryErrorTypes.SCHEME_ERROR).inc()
        # KIKIMR-4835 - we haven't specified status codes
        if _COLUMN_ALREADY_EXIST_ERROR_RE.search(error_message):
            return ColumnAlreadyExistsError(query, error_message)
        if _COLUMN_DOES_NOT_EXIST_ERROR_RE.search(error_message):
            return ColumnNotFoundError(query, error_message)
        return TableNotFoundError(query, error_message)

    if isinstance(ex, ydb.PreconditionFailed):
        metrics.query_errors_counter.labels(metrics.QueryErrorTypes.PRECONDITION_FAILED).inc()
        return PreconditionFailedError(query, error_message)

    if isinstance(ex, ydb.BadRequest):
        metrics.query_errors_counter.labels(metrics.QueryErrorTypes.BAD_REQUEST).inc()
        return BadRequest(query, error_message)

    if isinstance(ex, ydb.BadSession):
        metrics.query_errors_counter.labels(metrics.QueryErrorTypes.BAD_SESSION).inc()
        return BadSession(query, error_message)

    if isinstance(ex, ydb.DeadlineExceed):
        metrics.query_errors_counter.labels(metrics.QueryErrorTypes.DEADLINE_EXCEEDED)
        return YdbDeadlineExceeded(database_config, query, error_message)

    if isinstance(ex, ydb.GenericError):
        compile_error_strings = ("Failed to convert type:", "Pragma can't be set inside Kikimr query:")
        for check_string in compile_error_strings:
            if error_message.find(check_string) != -1:
                metrics.query_errors_counter.labels(metrics.QueryErrorTypes.FAILED_COMPILE_ERROR).inc()
                return CompileQueryError(query, error_message)

    log.warning("`%s` query has failed with an unknown KiKiMR error `%s`:\n%s", query, ex, error_message)
    metrics.query_errors_counter.labels(metrics.QueryErrorTypes.UNKNOWN).inc()
    return QueryError(query, "({}) {}".format(type(ex), error_message))


def _ydb_convert_transaction_mode(transaction_mode: TransactionMode) -> ydb.table.AbstractTransactionModeBuilder:
    if transaction_mode is None:
        transaction_mode = TransactionMode.SERIALIZABLE_READ_WRITE  # Default

    if transaction_mode == TransactionMode.SERIALIZABLE_READ_WRITE:
        return ydb.SerializableReadWrite()
    elif transaction_mode == TransactionMode.ONLINE_READ_ONLY_CONSISTENT:
        return ydb.OnlineReadOnly()
    elif transaction_mode == TransactionMode.ONLINE_READ_ONLY_INCONSISTENT:
        return ydb.OnlineReadOnly().with_allow_inconsistent_reads()
    elif transaction_mode == TransactionMode.STALE_READ_ONLY:
        return ydb.StaleReadOnly()
    else:
        raise LogicalError("Unknown transaction mode: `{}`", transaction_mode)


def _force_ydb_timeout(timeout):
    if timeout is None:
        return SERVICE_REQUEST_TIMEOUT + _ADDITIONAL_FORCE_YDB_TIMEOUT
    return timeout + _ADDITIONAL_FORCE_YDB_TIMEOUT


def _wrap_future_with_log(future: Future, function_name, details, timeout: float=_force_ydb_timeout(None),
                          request_type=metrics.QueryTypes.OTHER, log_warning_time: float=None):
    start = time.monotonic()
    wait_until = None if timeout is None else start + timeout
    try:
        while True:
            try:
                current_step_time = min(wait_until - time.monotonic(), _FUTURE_LOG_INTERVAL) if wait_until is not None \
                    else _FUTURE_LOG_INTERVAL
                return future.result(timeout=current_step_time)
            except FutureTimeoutError:
                wait_time = time.monotonic() - start
                if wait_until is None or time.monotonic() < wait_until:
                    log.warning("Waiting for %s (%s), now is %.1f seconds...", function_name, details, wait_time)
                else:
                    log.error("%s (%s) has timed out (%.1f seconds).", function_name, details, wait_time)
                    raise
    finally:
        request_time = time.monotonic() - start
        metrics.query_duration.labels(function_name, request_type).observe(request_time * 1000)
        if log_warning_time is not None and request_time >= log_warning_time:
            log.warning("`%s` (%s) query took %.1f seconds (force timeout %.1f seconds)", details, function_name, request_time, timeout)


def _retry_connection_error(func: Callable):
    func_name = func.__name__

    def _retry_connection_error_wrap(*args, _connection_retry_timeout: int, **kwargs):
        start = time.monotonic()
        sleep_time_generator = gen_exponential_backoff(1)
        try_number = 1

        kwargs["_connection_retry_timeout"] = _connection_retry_timeout
        while True:
            try:
                return func(*args, **kwargs)
            except ydb.ConnectionError as e:
                now = time.monotonic()
                if now - start > _connection_retry_timeout:
                    log.error("Retry ydb connection error timeout. Function '%s', args(%s, %s). Error %s (%s). Tried count: %s",
                              func_name, args, kwargs, e, type(e), try_number)
                    raise
                log.warn("Connection ydb error. Retry it. Function '%s', args(%s, %s). Error %s (%s). Tried count: %s...",
                         func_name, args, kwargs, e, type(e), try_number)
                sleep_time = next(sleep_time_generator)
                if (now + sleep_time) - start > _connection_retry_timeout:
                    sleep_time = start + _connection_retry_timeout - now
                time.sleep(sleep_time)
                try_number += 1

    return _retry_connection_error_wrap


@_retry_connection_error
def _wrap_ydb_commit(session: ydb.table.Session, transaction: ydb.table.TxContext, timeout: float,
                     log_warning_time: float=None, readonly_commit=False, *, _connection_retry_timeout: int):
    _ydb_session_must_in_use(session)
    settings = ydb.BaseRequestSettings().with_timeout(timeout)
    future = transaction.async_commit(settings=settings)
    request_type = metrics.QueryTypes.READ_ONLY_COMMIT if readonly_commit else metrics.QueryTypes.READ_WRITE_COMMIT
    return _wrap_future_with_log(future, "ydb_commit", "tx_id: {}".format(transaction.tx_id), _force_ydb_timeout(timeout),
                                 request_type, log_warning_time)


@_retry_connection_error
def _wrap_ydb_copy_table(session: ydb.table.Session, source: str, dest: str, timeout: float,
                         log_warning_time: float=None, *, _connection_retry_timeout: int):
    _ydb_session_must_in_use(session)
    settings = ydb.BaseRequestSettings().with_timeout(timeout)
    future = session.async_copy_table(source, dest, settings)
    return _wrap_future_with_log(future, "ydb_copy_table", "source: '{}', destination: '{}'".format(source, dest),
                                 _force_ydb_timeout(timeout), metrics.QueryTypes.OTHER, log_warning_time)


def _wrap_ydb_db_wait(config: KikimrEndpointConfig, driver: ydb.Driver, timeout, log_warning_time: float=None):
    future = driver.async_wait()
    try:
        return _wrap_future_with_log(future, "db_wait", "", timeout, metrics.QueryTypes.WAIT_DATABASE, log_warning_time)
    except FutureTimeoutError:
        raise RetryableConnectionError(config, "db_wait", "Timeout while wait database available.")


def _wrap_ydb_db_stop(driver: ydb.Driver, timeout: float, log_warning_time: float=None):
    pool = ThreadPoolExecutor(max_workers=1)
    future = pool.submit(driver.stop, timeout=timeout)
    try:
        result = _wrap_future_with_log(future, "db_wait_stop", "", timeout, metrics.QueryTypes.OTHER, log_warning_time)
    except FutureTimeoutError:
        raise KikimrError("Timeout while stop ydb db.")
    finally:
        pool.shutdown(wait=False)
    return result


@_retry_connection_error
def _wrap_ydb_describe_table(session: ydb.table.Session, path: str, timeout: float,
                             log_warning_time: float=None, *, _connection_retry_timeout: int) -> ydb.TableSchemeEntry:
    settings = ydb.BaseRequestSettings().with_timeout(timeout)
    future = session.async_describe_table(path, settings)
    return _wrap_future_with_log(future, "ydb_describe_table", "path: {}".format(path), _force_ydb_timeout(timeout),
                                 metrics.QueryTypes.OTHER,
                                 log_warning_time)


# noinspection PyProtectedMember
@_retry_connection_error
def _wrap_ydb_execute(session: ydb.table.Session, transaction: ydb.table.TxContext, query, detailed_query: str, append_commit, timeout: float,
                      parameters: dict = None, log_warning_time: float=None, request_type=None, *,
                      _connection_retry_timeout: int) -> List[ydb.convert.ResultSet]:
    _ydb_session_must_in_use(session)

    metrics.query_counter.inc()

    if detailed_query is None:
        detailed_query = query
    settings = ydb.BaseRequestSettings().with_timeout(timeout)
    future = transaction.async_execute(query, parameters=parameters, commit_tx=append_commit, settings=settings)

    thread_id = threading.current_thread().ident

    return _wrap_future_with_log(future, "ydb_execute",
        "thread_id: {}, session_id: {}, query: {}".format(thread_id, transaction.session_id, detailed_query),
                                 _force_ydb_timeout(timeout), request_type, log_warning_time)


@_retry_connection_error
def _wrap_ydb_execute_scheme(session: ydb.table.Session, query, timeout: float, log_warning_time: float=None, *,
                             _connection_retry_timeout: int):
    _ydb_session_must_in_use(session)

    metrics.query_counter.inc()

    settings = ydb.BaseRequestSettings().with_timeout(timeout)
    future = session.async_execute_scheme(query, settings=settings)

    return _wrap_future_with_log(future, "execute_scheme", query, _force_ydb_timeout(timeout), metrics.QueryTypes.OTHER,
                                 log_warning_time)


@_retry_connection_error
def _wrap_ydb_prepare_query(session: ydb.table.Session, query, timeout: float, log_warning_time: float=None, *,
                            _connection_retry_timeout: int):
    _ydb_session_must_in_use(session)
    settings = ydb.BaseRequestSettings().with_timeout(timeout)
    future = session.async_prepare(query, settings=settings)
    return _wrap_future_with_log(future, "prepare_query", query, _force_ydb_timeout(timeout),
                                 metrics.QueryTypes.PREPARE, log_warning_time)


@_retry_connection_error
def _wrap_ydb_rollback(session: ydb.table.Session, transaction: ydb.table.TxContext, timeout: float,
                       log_warning_time: float=None, *, _connection_retry_timeout: int):
    _ydb_session_must_in_use(session)
    settings = ydb.BaseRequestSettings().with_timeout(timeout)
    future = transaction.async_rollback(settings=settings)
    return _wrap_future_with_log(future, "ydb_rollback", "tx_id: {}".format(transaction.tx_id),
                                 _force_ydb_timeout(timeout), metrics.QueryTypes.ROLLBACK, log_warning_time)


@_retry_connection_error
def _wrap_ydb_list_directory(config: KikimrEndpointConfig, path: str, timeout: float, log_warning_time: float=None, *,
                             _connection_retry_timeout: int):
    driver = _ydb_driver_get(config)
    settings = ydb.BaseRequestSettings().with_timeout(timeout)
    future = driver.scheme_client.async_list_directory(path, settings=settings)
    return _wrap_future_with_log(future, "ydb_list_directory", "Path: '{}'".format(path), _force_ydb_timeout(timeout),
                                 metrics.QueryTypes.OTHER, log_warning_time)


@_retry_connection_error
def _wrap_ydb_make_directory(config: KikimrEndpointConfig, path: str, timeout: float, log_warning_time: float=None, *,
                             _connection_retry_timeout: int):
    driver = _ydb_driver_get(config)
    settings = ydb.BaseRequestSettings().with_timeout(timeout)
    future = driver.scheme_client.async_make_directory(path, settings=settings)
    return _wrap_future_with_log(future, "ydb_make_directory", "Path: '{}'".format(path), _force_ydb_timeout(timeout),
                                 metrics.QueryTypes.OTHER, log_warning_time)


@_retry_connection_error
def _wrap_ydb_remove_directory(config: KikimrEndpointConfig, path: str, timeout: float, log_warning_time: float=None, *,
                               _connection_retry_timeout: int):
    driver = _ydb_driver_get(config)
    settings = ydb.BaseRequestSettings().with_timeout(timeout)
    future = driver.scheme_client.async_remove_directory(path, settings=settings)
    return _wrap_future_with_log(future, "ydb_remove_directory", "Path: '{}'".format(path), _force_ydb_timeout(timeout),
                                 metrics.QueryTypes.OTHER, log_warning_time)


def _wrap_ydb_session_delete(session: ydb.table.Session, timeout: float, *, _connection_retry_timeout: int):
    settings = ydb.BaseRequestSettings().with_timeout(timeout)
    future = session.async_delete(settings)
    return _wrap_future_with_log(future, "ydb_delete_session", "session_id: '{}'".format(session.session_id),
                                 _force_ydb_timeout(timeout), metrics.QueryTypes.OTHER, 1)


@_retry_connection_error
def _wrap_ydb_session_create(session: ydb.table.Session, timeout: float, *, _connection_retry_timeout: int) \
        -> ydb.table.Session:
    settings = ydb.BaseRequestSettings().with_timeout(timeout)
    future = session.async_create(settings)
    return _wrap_future_with_log(future, "ydb_create_session", "", _force_ydb_timeout(timeout),
                                 metrics.QueryTypes.OTHER, 1)
