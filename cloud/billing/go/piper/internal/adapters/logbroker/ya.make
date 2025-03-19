GO_LIBRARY()

SRCS(
    adapter.go
    errors.go
    push.go
    rate.go
    retry.go
)

GO_TEST_SRCS(
    ShardProducer_test.go
    common_test.go
    push_test.go
    rate_test.go
)

END()

RECURSE(gotest)
