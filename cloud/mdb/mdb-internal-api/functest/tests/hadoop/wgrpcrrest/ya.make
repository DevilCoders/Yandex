GO_TEST()

OWNER(g:mdb)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/mdb-internal-api/functest/tests/functest.inc)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/mdb-internal-api/functest/tests/wgrpcrrest.inc)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/mdb-internal-api/functest/tests/hadoop/tests.inc)

GO_TEST_SRCS(run_test.go)

END()
