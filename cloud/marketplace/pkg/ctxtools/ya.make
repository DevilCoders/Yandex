GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(context.go)

GO_TEST_SRCS(
    context_test.go
)

END()

RECURSE_FOR_TESTS(gotest)
