GO_TEST()

OWNER(g:mdb)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/mdb-internal-api/functest/tests/functest.inc)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/mdb-internal-api/functest/tests/wgrpcrrest.inc)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/mdb-internal-api/functest/tests/redis/tests.inc)

REQUIREMENTS(ram:9)

GO_TEST_SRCS(run_test.go)

END()
