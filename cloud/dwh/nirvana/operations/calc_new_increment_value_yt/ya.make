OWNER(g:cloud-dwh)

PY3_PROGRAM()

PEERDIR(
    library/python/nirvana
    yql/library/python
    cloud/dwh/lms
    yt/python/client
    contrib/python/pytz
)

PY_SRCS(MAIN __init__.py)

END()
