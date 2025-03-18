GO_LIBRARY()

OWNER(
    g:go-library
    gzuykov
)

SRCS(
    counter.go
    gauge.go
    histogram.go
    registry.go
    registry_opts.go
    timer.go
    vec.go
)

GO_TEST_SRCS(
    counter_test.go
    gauge_test.go
    histogram_test.go
    registry_test.go
    timer_test.go
    vec_test.go
)

END()

RECURSE(gotest)
