GO_LIBRARY()

OWNER(g:go-library)

SRCS(yolint.go)

GO_TEST_SRCS(yolint_test.go)

END()

RECURSE(gotest)
