OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
    cloud/dwh/nirvana/vh/common
    library/python/startrek_python_client
    yt/python/client_with_rpc
)

PY_SRCS(__init__.py)

END()
