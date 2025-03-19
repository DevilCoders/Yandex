GO_LIBRARY()

OWNER(g:cloud-billing)

SRCS(
    config.go
    consumer.go
    counters.go
    errors.go
    infly.go
    lb_events.go
    service.go
    util.go
    writer.go
)

GO_TEST_SRCS(
    counters_test.go
    errors_test.go
    infly_test.go
    logbroker_test.go
    service_test.go
    util_test.go
    writer_test.go
)

END()

RECURSE(
    gotest
    lbtypes
)
