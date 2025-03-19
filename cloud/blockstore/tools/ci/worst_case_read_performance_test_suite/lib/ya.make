PY3_LIBRARY()

OWNER(g:cloud-nbs)

PY_SRCS(
    __init__.py
    errors.py
    yt.py
)

PEERDIR(
    yt/python/client
)

END()

