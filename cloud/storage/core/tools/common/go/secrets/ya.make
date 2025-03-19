GO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    iam.go
    secrets.go
)

GO_TEST_SRCS(
    secrets_test.go
)

END()

RECURSE_FOR_TESTS(tests)
