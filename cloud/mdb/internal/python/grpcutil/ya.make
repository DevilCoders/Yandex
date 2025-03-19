PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    contrib/libs/grpc/src/python/grpcio_status
    cloud/mdb/dbaas_python_common
    cloud/mdb/internal/python/logs
)

PY_SRCS(
    __init__.py
    service.py
    exceptions.py
    retries.py
)

END()
