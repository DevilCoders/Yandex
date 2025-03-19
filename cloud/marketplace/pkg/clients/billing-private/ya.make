GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    billing_accounts.go
    client.go
    errors.go
    interfaces.go
    session.go
    skus.go
)

GO_TEST_SRCS(billing_accounts_test.go)

END()

RECURSE(dev)

RECURSE_FOR_TESTS(gotest)
