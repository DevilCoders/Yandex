PY2_PROGRAM(update_tickets)

OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
)

PEERDIR(
    contrib/python/requests
    library/python/startrek_python_client
    antiadblock/tasks/money_tickets_updater/plugins
    antiadblock/tasks/tools
)

END()
