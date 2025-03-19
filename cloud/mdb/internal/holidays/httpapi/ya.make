GO_LIBRARY()

OWNER(g:mdb)

SRCS(httpapi.go)

GO_TEST_SRCS(httpapi_test.go)

END()

RECURSE(gotest)
