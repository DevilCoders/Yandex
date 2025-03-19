OWNER(
    g:cloud-iam
)

GO_LIBRARY()

SRCS(
    client.go
)

GO_TEST_SRCS(
    client_test.go
)

END()

RECURSE_FOR_TESTS(gotest)
