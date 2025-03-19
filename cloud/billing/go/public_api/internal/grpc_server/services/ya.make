GO_LIBRARY()

SRCS(
    access.go
    auth.go
    billable_object.go
    billing_account.go
    budget.go
    customer.go
    errors.go
    helpers.go
    operation.go
    service.go
    sku.go
    test_helpers.go
)

GO_TEST_SRCS(
    access_test.go
    billable_object_test.go
    billing_account_test.go
    generate_test.go
    operation_test.go
    service_test.go
    sku_test.go
)

END()

RECURSE(
    gotest
    mocks
)
