GO_LIBRARY()

OWNER(
    prime
    g:go-library
    g:solomon
)

SRCS(
    alert.go
    client.go
    cluster.go
    differ.go
    generic.go
    options.go
    service.go
    shard.go
    sync.go
)

GO_TEST_SRCS(differ_test.go)

GO_XTEST_SRCS(
    client_example_test.go
    client_test.go
    sync_test.go
)

END()

RECURSE(gotest)
