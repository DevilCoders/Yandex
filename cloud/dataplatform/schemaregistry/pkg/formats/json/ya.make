GO_LIBRARY()

OWNER(tserakhau)

SRCS(
    provider.go
    schema.go
)

GO_XTEST_SRCS(provider_test.go)

END()

RECURSE(gotest)
