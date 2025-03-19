PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    cloud/bitbucket/private-api/yandex/cloud/priv/loadbalancer/v1
    cloud/mdb/internal/python/grpcutil
    cloud/mdb/internal/python/logs
)

PY_SRCS(
    __init__.py
    models.py
    api.py
)

END()
