GO_LIBRARY()

OWNER(g:go-library)

SRCS(output.go)

GO_XTEST_SRCS(output_test.go)

END()

RECURSE(gotest)
