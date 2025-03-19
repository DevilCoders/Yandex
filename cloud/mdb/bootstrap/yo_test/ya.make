GO_TEST()

OWNER(g:mdb)

DATA(arcadia/cloud/mdb/bootstrap)

INCLUDE(${ARCADIA_ROOT}/library/go/test/checks/yofix/run.inc)

TEST_CWD(cloud/mdb/bootstrap)

GO_TEST_SRCS(yo_test.go)

END()
