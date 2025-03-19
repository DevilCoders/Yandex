PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas_metadb/recipes/bare/recipe.inc)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/internal/dbteststeps/deps.inc)

FORK_SUBTESTS()

REQUIREMENTS(cpu:4)

PEERDIR(
    cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1/awscompatibility
    contrib/python/httmock
    contrib/python/PyHamcrest
    contrib/python/pytest-mock
    contrib/python/mock
    contrib/python/moto
    contrib/python/boto
    cloud/mdb/dbaas_worker/internal
    library/python/testing/yatest_common
    contrib/libs/grpc/src/python/grpcio_status
)

DATA(arcadia/cloud/mdb/dbaas_worker)

TEST_SRCS(
    db.py
    utils.py
    test_metadb_alerts.py
)

END()
