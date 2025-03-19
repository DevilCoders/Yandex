GO_TEST()

OWNER(g:mdb)

DATA(arcadia/cloud/mdb/image-releaser)

INCLUDE(${ARCADIA_ROOT}/library/go/test/checks/yofix/run.inc)

TEST_CWD(cloud/mdb/image-releaser)

GO_TEST_SRCS(yo_test.go)

END()
