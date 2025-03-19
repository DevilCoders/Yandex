OWNER(g:mdb)

PY3TEST()

STYLE_PYTHON()

PEERDIR(
    library/python/testing/yatest_common
    contrib/python/PyHamcrest
    contrib/python/jsonpath-rw
    contrib/python/pytest-mock
    contrib/python/requests-mock
    cloud/mdb/tools/bot/bot_lib
)

TEST_SRCS(
    __init__.py
    enrich.py
    bot_client.py
)

DATA(
    arcadia/cloud/mdb/tools/bot/tests/srv.json
    arcadia/cloud/mdb/tools/bot/tests/900918002.json
    arcadia/cloud/mdb/tools/bot/tests/900918363.json
)

SIZE(SMALL)

END()
