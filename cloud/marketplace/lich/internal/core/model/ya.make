GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    billing_accounts.go
    license_rules.go
    products.go
    resource_specs.go
)

GO_TEST_SRCS(
    license_rules_test.go
)

END()

RECURSE(
    gotest
)
