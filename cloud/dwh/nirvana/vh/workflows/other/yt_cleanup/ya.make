OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
    cloud/dwh/nirvana/vh/common
    yt/python/client_with_rpc
    contrib/python/pytz
)

PY_SRCS(__init__.py)

END()
