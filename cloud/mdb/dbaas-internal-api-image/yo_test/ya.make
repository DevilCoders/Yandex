GO_TEST()

OWNER(g:mdb)

DATA(arcadia/cloud/mdb/dbaas-internal-api-image)

INCLUDE(${ARCADIA_ROOT}/library/go/test/checks/yofix/run.inc)

TEST_CWD(cloud/mdb/dbaas-internal-api-image)

GO_TEST_SRCS(yo_test.go)

END()
