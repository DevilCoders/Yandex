PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PY_NAMESPACE(test.mocks)

PEERDIR(
    cloud/mdb/dbaas_worker/internal
    cloud/mdb/dbaas_python_common
    contrib/python/httmock
    contrib/python/mock
    contrib/python/moto
    contrib/python/boto
)

ALL_PY_SRCS(RECURSIVE)

END()
