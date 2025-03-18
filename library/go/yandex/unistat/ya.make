GO_LIBRARY()

OWNER(
    ivaxer
    g:golovan
    g:go-library
)

SRCS(
    histogram.go
    number.go
    registry.go
    unistat.go
)

GO_TEST_SRCS(
    histogram_test.go
    number_test.go
    registry_test.go
    unistat_test.go
)

END()

RECURSE(
    aggr
    example_server
    gotest
)
