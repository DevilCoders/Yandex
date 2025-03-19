GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(conv.go)

GO_TEST_SRCS(conv_test.go)

END()

RECURSE_FOR_TESTS(gotest)
