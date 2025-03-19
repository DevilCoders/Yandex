GO_TEST()

OWNER(g:mdb)

DATA(arcadia/cloud/mdb/dbaas_metadb)

INCLUDE(${ARCADIA_ROOT}/library/go/test/checks/yofix/run.inc)

TEST_CWD(cloud/mdb/dbaas_metadb)

GO_TEST_SRCS(yo_test.go)

END()
