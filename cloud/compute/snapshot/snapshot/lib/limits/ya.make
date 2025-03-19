GO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(limits.go)

GO_TEST_SRCS(limits_test.go)

END()

RECURSE(gotest)
