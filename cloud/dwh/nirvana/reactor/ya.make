OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    library/python/reactor/client

    cloud/dwh/nirvana/config
)

PY_SRCS(
    __init__.py
)

END()
