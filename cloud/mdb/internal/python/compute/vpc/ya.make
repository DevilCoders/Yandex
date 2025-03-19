PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

FORK_TESTS()

PEERDIR(
    cloud/bitbucket/private-api/yandex/cloud/priv/vpc/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/vpc/v1/inner
    cloud/bitbucket/private-api/yandex/cloud/priv/servicecontrol/v1
    cloud/mdb/dbaas_python_common
    cloud/mdb/internal/python/grpcutil
)

PY_SRCS(
    __init__.py
    api.py
    models.py
)

END()
