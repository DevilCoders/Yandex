GO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(sharedlock.go)

GO_TEST_SRCS(sharedlock_test.go)

END()

RECURSE(gotest)
