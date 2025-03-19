PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    cloud/bitbucket/private-api/yandex/cloud/priv/compute/v1
    cloud/mdb/dbaas_python_common
    cloud/mdb/internal/python/grpcutil
    cloud/mdb/internal/python/compute
    cloud/mdb/internal/python/logs
)

ALL_PY_SRCS()

END()

RECURSE_FOR_TESTS(
    test
)
