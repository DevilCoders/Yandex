GO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(server.go)

GO_TEST_SRCS(server_test.go)

END()

RECURSE(gotest)
