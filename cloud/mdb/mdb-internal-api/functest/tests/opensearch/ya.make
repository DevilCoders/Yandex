GO_TEST()

OWNER(g:mdb)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/mdb-internal-api/functest/tests/functest.inc)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/mdb-internal-api/functest/tests/wgrpcrgrpc.inc)

ENV(_SETUP_GODOG_FEATURE_PATHS=cloud/mdb/mdb-internal-api/functest/features/opensearch)

GO_TEST_SRCS(run_test.go)

END()
