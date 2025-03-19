GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    backoff.go
    config.go
    retry.go
)

GO_TEST_SRCS(backoff_test.go)

GO_XTEST_SRCS(benchmark_test.go)

END()

RECURSE(gotest)
