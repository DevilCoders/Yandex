PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    cloud/mdb/datacloud/private_api/datacloud/network/v1
    cloud/mdb/dbaas_python_common
    cloud/mdb/internal/python/grpcutil
)

ALL_PY_SRCS()

END()
