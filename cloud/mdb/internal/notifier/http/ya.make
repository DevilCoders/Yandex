GO_LIBRARY()

OWNER(g:mdb)

SRCS(notifier.go)

GO_XTEST_SRCS(notifier_test.go)

END()

RECURSE(gotest)
