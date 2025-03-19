"""Metrics for KiKiMR queries."""

from yc_common.metrics import Metric, MetricTypes


class QueryErrorTypes:
    CONNECTION = "connection_error"
    INTERNAL = "internal_error"
    BAD_SESSION = "bad_session"

    SCHEME_ERROR = "scheme_error"
    PRECONDITION_FAILED = "precondition_failed"
    INVALIDATED_LOCKS = "invalidated_locks"
    PENDING_QUERY = "pending_query_error"
    BAD_REQUEST = "bad_request"
    FAILED_COMPILE_ERROR = "failed_compile_error"
    OVERLOAD = "overloaded"
    DEADLINE_EXCEEDED = "deadline_exceeded"

    UNKNOWN = "unknown_error"


query_counter = Metric(
    MetricTypes.COUNTER,
    "kikimr_requests",
    doc="Counter for KiKiMR client requests.")

query_errors_counter = Metric(
    MetricTypes.COUNTER,
    "kikimr_request_errors",
    ["error_type"],
    "Counter for KiKiMR client request errors.")


class QueryPreparedCounterCache:
    COMPILE = "compiled"
    CACHED = "cached"


query_prepared_cache_counter = Metric(
    MetricTypes.COUNTER,
    name="kikimr_prepare_requests",
    label_names=["compiled_cache"],
    doc="Doc counter of prepared queries."
)

ydb_connections_pool = Metric(
    MetricTypes.GAUGE,
    name="kikimr_sessions_using",
    label_names=["ydb_root"],
    doc="Count of current used sessions"
)

ydb_leak_sessions = Metric(
    MetricTypes.COUNTER,
    name="kikimr_sessions_leak",
    label_names=["ydb_root"],
    doc="Count of leaked ydb session"
)

class QueryBlockErrorTypes:
    CONNECTION = "connection_error"
    QUERY = "query_error"
    UNKNOWN = "unknown_error"


query_blocks_counter = Metric(
    MetricTypes.COUNTER,
    "kikimr_query_blocks",
    doc="Counter for KiKiMR query blocks.")

query_block_errors_counter = Metric(
    MetricTypes.COUNTER,
    "kikimr_query_block_errors",
    ["error_type"],
    "Counter for KiKiMR query block errors.")

tx_buckets_ms = (50.0, 100.0, 200.0, 400.0, 800.0, 1000.0) + \
    (2000.0, 4000.0, 6000.0, 8000.0, 10000.0) + \
    (20000.0, 60000.0, 120000.0, 240000.0, 600000.0)

query_duration = Metric(
    MetricTypes.HISTOGRAM,
    "kikimr_query_duration",
    ["method", "type"],
    "KiKiMR query duration in ms.",
    buckets=tx_buckets_ms)

transaction_duration = Metric(
    MetricTypes.HISTOGRAM,
    "kikimr_transaction_duration",
    ["type"],
    "KiKiMR transaction duration in ms.",
    buckets=tx_buckets_ms)


class QueryTypes:
    PREPARE = "prepare"
    READ_ONLY_COMMIT = "ro_commit"
    READ_WRITE_COMMIT = "rw_commit"
    SELECTS = "selects"
    UPDATES = "updates"
    ROLLBACK = "rollback"
    WAIT_DATABASE = "wait_database"
    OTHER = "other"


class TxTypes:
    COMMITTED = "commited"
    ROLLED_BACK = "rolled-back"
    FAILED = "failed"
