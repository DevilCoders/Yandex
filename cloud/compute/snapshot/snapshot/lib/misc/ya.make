GO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    compound_context.go
    constants.go
    custom_metric_avg.go
    errors.go
    healthchecks.go
    labeled_metric.go
    max_timeout_context.go
    metrics.go
    retry.go
    table_ops.go
    util.go
)

GO_TEST_SRCS(
    compound_context_test.go
    labeled_metric_test.go
    max_timeout_context_test.go
    metrics_test.go
)

END()

RECURSE(gotest)
