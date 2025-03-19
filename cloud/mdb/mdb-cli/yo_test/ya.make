GO_TEST()

OWNER(g:mdb)

DATA(arcadia/cloud/mdb/mdb-cli)

INCLUDE(${ARCADIA_ROOT}/library/go/test/checks/yofix/run.inc)

TEST_CWD(cloud/mdb/mdb-cli)

GO_TEST_SRCS(yo_test.go)

END()
