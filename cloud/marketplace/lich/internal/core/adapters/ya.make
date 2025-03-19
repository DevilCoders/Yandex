GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    billing.go
    errors.go
)

GO_TEST_SRCS(
    billing_test.go
)

END()

RECURSE(
    db
    gotest
    mocks
    structs
)
