GO_LIBRARY()

OWNER(g:mdb)

SRCS(sqlserver.go)

GO_TEST_SRCS(pillars_test.go)

END()

RECURSE(gotest)
