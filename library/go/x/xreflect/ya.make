GO_LIBRARY()

OWNER(g:go-library)

SRCS(assign.go)

GO_XTEST_SRCS(assign_test.go)

END()

RECURSE(gotest)
