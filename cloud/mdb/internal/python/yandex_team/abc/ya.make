PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    cloud/bitbucket/private-api/yandex/cloud/priv/team/integration/v1
    cloud/mdb/dbaas_python_common
    cloud/mdb/internal/python/grpcutil
)

ALL_PY_SRCS()

END()
