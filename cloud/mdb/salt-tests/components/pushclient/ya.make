PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    cloud/mdb/salt/salt/components/pushclient/parsers
)

TEST_SRCS(
    test_clickhouse_parser.py
    test_log_parser.py
    test_mongodb_audit_log_parser.py
    test_mongodb_parser.py
    test_mysql_parser.py
    test_redis_parser.py
)

TIMEOUT(60)

SIZE(SMALL)

END()
