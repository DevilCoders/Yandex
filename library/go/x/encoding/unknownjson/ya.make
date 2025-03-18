GO_LIBRARY()

OWNER(
    prime
    g:go-library
)

SRCS(
    marshal.go
    store.go
    typcache.go
    unmarshal.go
)

GO_TEST_SRCS(roundtrip_test.go)

GO_XTEST_SRCS(example_test.go)

END()

RECURSE(gotest)
