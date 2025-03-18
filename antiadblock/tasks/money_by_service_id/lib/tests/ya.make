PY3TEST()

OWNER(g:antiadblock)

TEST_SRCS(
    test.py
    antiadblock/tasks/money_by_service_id/lib/lib.py
)

PEERDIR(
    contrib/python/pandas
    antiadblock/tasks/tools
    antiadblock/tasks/money_by_service_id/lib
)

SIZE(SMALL)

END()
