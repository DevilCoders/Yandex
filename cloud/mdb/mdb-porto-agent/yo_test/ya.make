GO_TEST()

OWNER(g:mdb)

DATA(arcadia/cloud/mdb/mdb-porto-agent)

INCLUDE(${ARCADIA_ROOT}/library/go/test/checks/yofix/run.inc)

TEST_CWD(cloud/mdb/mdb-porto-agent)

GO_TEST_SRCS(yo_test.go)

END()
