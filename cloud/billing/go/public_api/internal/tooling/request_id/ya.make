GO_LIBRARY()

SRCS(
    grpc.go
    http.go
    utils.go
)

GO_TEST_SRCS(utils_test.go)

END()

RECURSE(gotest)
