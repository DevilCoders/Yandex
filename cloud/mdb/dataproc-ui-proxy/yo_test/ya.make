GO_TEST()

OWNER(g:mdb-dataproc)

DATA(arcadia/cloud/mdb/dataproc-ui-proxy)

INCLUDE(${ARCADIA_ROOT}/library/go/test/checks/yofix/run.inc)

TEST_CWD(cloud/mdb/dataproc-ui-proxy)

GO_TEST_SRCS(yo_test.go)

END()
