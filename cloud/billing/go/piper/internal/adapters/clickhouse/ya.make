GO_LIBRARY()

SRCS(
    adapter.go
    errors.go
    pools.go
    push.go
    retry.go
)

GO_TEST_SRCS(
    adapter_test.go
    common_test.go
    push_test.go
    retry_test.go
)

END()

RECURSE(gotest)
