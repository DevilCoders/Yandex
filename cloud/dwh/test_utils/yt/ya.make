OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    yql/library/python
    yt/python/client
)

PY_SRCS(
    conftest.py
    misc.py
)

END()
