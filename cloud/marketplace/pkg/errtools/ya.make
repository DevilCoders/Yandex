GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(stack.go)

GO_TEST_SRCS(stack_test.go)

END()

RECURSE_FOR_TESTS(gotest)
