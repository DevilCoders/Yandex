GO_LIBRARY()

OWNER(
    g:strm-admin
    g:traffic
)

SRCS(
    common.go
    origins.go
    origins_validation.go
    resource.go
    resource_rule.go
    resource_rule_validation.go
    resource_validation.go
    validation_utils.go
)

GO_TEST_SRCS(
    common_test.go
    resource_test.go
    resource_validation_test.go
)

END()

RECURSE(gotest)
