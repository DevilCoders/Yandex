OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PEERDIR(
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/v1
    cloud/mdb/e2e-tests/dbaas_e2e/scenarios
    cloud/mdb/internal/python/compute/iam/jwt
    cloud/mdb/internal/python/grpcutil
    contrib/libs/grpc/src/python/grpcio
)

PY_SRCS(
    NAMESPACE dbaas_e2e
    __init__.py
    config.py
    internal_api.py
    utils.py
)

END()

RECURSE(
    scenarios
)
