GO_LIBRARY()

OWNER(
    g:go-library
    g:alice_iot
)

SRCS(
    client.go
    filter.go
)

GO_TEST_SRCS(filter_test.go)

END()

RECURSE(
    gotest
    httpxiva
)
