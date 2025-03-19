GO_TEST()

OWNER(g:mdb-dataproc)

DATA(arcadia/cloud/mdb/dataproc-agent)

INCLUDE(${ARCADIA_ROOT}/library/go/test/checks/yofix/run.inc)

TEST_CWD(cloud/mdb/dataproc-agent)

GO_TEST_SRCS(yo_test.go)

END()
