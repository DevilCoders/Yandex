GO_LIBRARY()

OWNER(g:cloud-billing)

SRCS(sentinel.go)

GO_TEST_SRCS(sentinel_test.go)

END()

RECURSE(gotest)
