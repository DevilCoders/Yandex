GO_LIBRARY()

SRCS(
    json.go
    metrics.go
    types_easyjson.go
)

GO_TEST_SRCS(
    metrics_test.go
)

END()

RECURSE(
    marshal
    gotest
)
