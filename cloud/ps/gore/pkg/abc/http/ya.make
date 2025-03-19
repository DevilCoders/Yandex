GO_LIBRARY()

OWNER(g:cloud-ps)

SRCS(http.go)

GO_TEST_SRCS(http_test.go)

END()

RECURSE(gotest)
