PY2_LIBRARY()

OWNER(akastornov)

PEERDIR(
    library/python/cityhash
    ydb/public/sdk/python
    contrib/python/futures
)

PY_SRCS(
    ydb.py
)

END()
