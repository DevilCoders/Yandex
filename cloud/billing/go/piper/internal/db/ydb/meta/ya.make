GO_LIBRARY()

SRCS(
    billing_account.go
    common.go
    conversion_rates.go
    queries.go
    schemas.go
    sku.go
    units.go
)

GO_TEST_SRCS(
    billing_account_test.go
    conversion_rates_test.go
    main_test.go
    schemas_test.go
    sku_test.go
    units_test.go
)

END()

RECURSE(gotest)
