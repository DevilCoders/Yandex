GO_LIBRARY()

OWNER(g:mdb)

SRCS(mysyncinfo.go)

GO_TEST_SRCS(mysyncinfo_test.go)

END()

RECURSE(gotest)
