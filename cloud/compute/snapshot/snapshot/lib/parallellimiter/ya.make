GO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(parallellimiter.go)

GO_TEST_SRCS(parallellimiter_test.go)

END()

RECURSE(gotest)
