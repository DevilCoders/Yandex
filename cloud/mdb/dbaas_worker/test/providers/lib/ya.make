PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

ALL_PY_SRCS()

PEERDIR(
    cloud/mdb/dbaas_worker/test/mocks
    contrib/python/pytest
    contrib/python/pytest-mock
)

END()
