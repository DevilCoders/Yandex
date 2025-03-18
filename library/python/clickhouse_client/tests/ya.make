PY2TEST()

OWNER(
    g:statinfra
)

PEERDIR(
    contrib/python/responses
    library/python/clickhouse_client
)

SET(PREFIX library/python/clickhouse_client/tests)

TEST_SRCS(
    ${PREFIX}/conftest.py
    ${PREFIX}/test_connection.py
    ${PREFIX}/test_cursor.py
)

DATA(
    arcadia/${PREFIX}/config/conn_params.json
    arcadia/${PREFIX}/config/test_data.json
)

END()

