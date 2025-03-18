PY2_PROGRAM(support_money_loss)

OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
)

PEERDIR(
    library/python/statface_client
    library/python/startrek_python_client
    library/python/charts_notes
    antiadblock/tasks/tools
)

END()
