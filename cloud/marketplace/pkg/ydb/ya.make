GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    connector.go
    options.go
    status.go
    types.go
)

GO_TEST_SRCS(
    base_test.go
    connector_test.go
)

END()

RECURSE(gotest)
