PY2_PROGRAM(monitoring_update)

OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
    money_monitoring.py
)

PEERDIR(
    antiadblock/tasks/tools
)

END()
