PY23_LIBRARY()

OWNER(
    pg
    borman
)

PEERDIR(
    contrib/python/dateutil
    yt/python/client_lite
)

PY_SRCS(
    __init__.py
    lock.py
    ttl.py
    db.py
)

END()
