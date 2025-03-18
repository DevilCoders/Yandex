PY3_LIBRARY()

OWNER(g:antiadblock)

PY_SRCS(
    lib.py
)

PEERDIR(
    contrib/python/pandas
    antiadblock/tasks/tools
)

END()
