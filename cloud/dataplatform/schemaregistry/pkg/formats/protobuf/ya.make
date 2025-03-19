GO_LIBRARY()

OWNER(tserakhau)

SRCS(
    compatibility.go
    error.go
    provider.go
    schema.go
    utils.go
)

GO_XTEST_SRCS(
    # compatibility_test.go
    # provider_test.go
    # schema_test.go
)

END()

RECURSE(gotest)
