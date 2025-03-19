PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    cloud/bitbucket/private-api/yandex/cloud/priv/dns/v1
    cloud/mdb/dbaas_python_common
    cloud/mdb/internal/python/grpcutil
    cloud/mdb/internal/python/compute
    cloud/mdb/internal/python/logs
)

PY_SRCS(
    __init__.py
    api.py
    models.py
)

END()

RECURSE_FOR_TESTS(
    test/mypy
    test/units
)

