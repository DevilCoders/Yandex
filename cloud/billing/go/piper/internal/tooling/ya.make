GO_LIBRARY()

SRCS(
    callstack.go
    context.go
    context_logger.go
    context_metrics.go
    context_modify.go
    grpc.go
    impl.go
    interface.go
    types.go
    util.go
)

GO_TEST_SRCS(
    callstack_test.go
    common_test.go
    features_test.go
    logging_test.go
    metrics_test.go
    tracing_test.go
)

END()

RECURSE(
    dev
    features
    gotest
    logf
    logging
    metrics
    tracetag
    tracing
)
