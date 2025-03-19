GO_LIBRARY()

SRCS(
    errors.go
    params.go
    query.go
    rows.go
    scheme.go
    template.go
    types.go
)

GO_TEST_SRCS(
    types_test.go
)

END()

RECURSE(
    gotest
)
