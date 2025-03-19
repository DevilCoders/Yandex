PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    cloud/mdb/internal/python/grpcutil
    cloud/mdb/dbaas_python_common
)

END()

RECURSE(
    abc
)
