GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    aggregator.go
    ready.go
)

GO_TEST_SRCS(ready_test.go)

END()

RECURSE(
    gotest
    mocks
)
