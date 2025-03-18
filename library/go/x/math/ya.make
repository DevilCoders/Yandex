GO_LIBRARY()

OWNER(g:go-library)

SRCS(utils.go)

GO_TEST_SRCS(utils_test.go)

END()

RECURSE(gotest)
