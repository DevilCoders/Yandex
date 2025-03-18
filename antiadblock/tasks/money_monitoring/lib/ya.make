PY3_LIBRARY()

OWNER(g:antiadblock)

PY_SRCS(
    data_checks.py
)

PEERDIR(
    library/python/yt
    yql/library/python
    library/python/startrek_python_client
    contrib/python/pandas
    antiadblock/tasks/tools
)

END()
