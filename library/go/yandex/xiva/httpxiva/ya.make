GO_LIBRARY()

OWNER(
    g:go-library
    g:alice_iot
)

SRCS(
    client.go
    options.go
)

GO_XTEST_SRCS(example_test.go)

END()

RECURSE(gotest)
