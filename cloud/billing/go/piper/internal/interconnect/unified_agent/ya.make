GO_LIBRARY()

SRCS(
    client.go
    interface.go
)

GO_TEST_SRCS(client_test.go)

END()

RECURSE(mocks)
