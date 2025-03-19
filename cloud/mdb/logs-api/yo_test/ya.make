GO_TEST()

OWNER(g:mdb)

DATA(arcadia/cloud/mdb/logs-api)

INCLUDE(${ARCADIA_ROOT}/library/go/test/checks/yofix/run.inc)

TEST_CWD(cloud/mdb/logs-api)

GO_TEST_SRCS(yo_test.go)

END()
