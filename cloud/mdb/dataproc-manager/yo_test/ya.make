GO_TEST()

OWNER(g:mdb-dataproc)

DATA(arcadia/cloud/mdb/dataproc-manager)

INCLUDE(${ARCADIA_ROOT}/library/go/test/checks/yofix/run.inc)

TEST_CWD(cloud/mdb/dataproc-manager)

GO_TEST_SRCS(yo_test.go)

END()
