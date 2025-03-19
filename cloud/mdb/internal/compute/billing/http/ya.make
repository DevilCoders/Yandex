GO_LIBRARY()

OWNER(g:mdb)

SRCS(http.go)

GO_TEST_SRCS(http_test.go)

END()

RECURSE(gotest)
