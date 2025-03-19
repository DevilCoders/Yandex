GO_TEST()

OWNER(g:mdb)

DATA(arcadia/cloud/mdb/grpc-ping)

INCLUDE(${ARCADIA_ROOT}/library/go/test/checks/yofix/run.inc)

TEST_CWD(cloud/mdb/grpc-ping)

GO_TEST_SRCS(yo_test.go)

END()
