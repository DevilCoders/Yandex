PY3_PROGRAM(money_monitoring)

OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
)

PEERDIR(
    antiadblock/tasks/tools
    antiadblock/tasks/money_monitoring/lib
)

END()
