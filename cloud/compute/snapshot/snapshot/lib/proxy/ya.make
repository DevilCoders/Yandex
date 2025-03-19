GO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(proxy.go)

GO_TEST_SRCS(proxy_test.go)

END()

RECURSE(gotest)
