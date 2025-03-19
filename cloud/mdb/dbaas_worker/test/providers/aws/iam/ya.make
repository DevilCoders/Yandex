PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    cloud/mdb/dbaas_worker/internal
    cloud/mdb/dbaas_worker/test/mocks
    cloud/mdb/dbaas_worker/test/providers/lib
    contrib/python/moto
    contrib/python/boto
    contrib/python/PyHamcrest
)

TEST_SRCS(test_iam.py)

END()
