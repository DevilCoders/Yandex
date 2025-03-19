PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    cloud/bitbucket/private-api/yandex/cloud/priv/operation
    cloud/mdb/dbaas_python_common
    cloud/mdb/internal/python/grpcutil
    cloud/mdb/internal/python/compute
    cloud/mdb/internal/python/logs
)

PY_SRCS(
    __init__.py
    api.py
)

END()
