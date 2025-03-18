GO_LIBRARY()

OWNER(
    g:go-library
    gzuykov
)

SRCS(
    struct.go
    validator.go
    value.go
)

GO_TEST_SRCS(
    struct_test.go
    validator_test.go
)

END()

RECURSE(
    gotest
    inspection
    rule
    tests
)
