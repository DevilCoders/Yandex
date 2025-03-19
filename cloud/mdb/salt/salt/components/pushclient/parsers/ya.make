PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PY_SRCS(
    TOP_LEVEL
    clickhouse_server_log_parser.py
    conftest.py
    log_parser.py
    mongodb_log_parser.py
    mysql_audit_log_parser.py
    mysql_error_log_parser.py
    mysql_general_log_parser.py
    mysql_slow_query_log_parser.py
    redis_server_log_parser.py
)

NEED_CHECK()

END()
