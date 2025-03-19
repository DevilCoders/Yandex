GO_TEST()

OWNER(g:mdb)

DATA(arcadia/cloud/mdb/deploy)

INCLUDE(${ARCADIA_ROOT}/library/go/test/checks/yofix/run.inc)

TEST_CWD(cloud/mdb/deploy)

GO_TEST_SRCS(yo_test.go)

END()
