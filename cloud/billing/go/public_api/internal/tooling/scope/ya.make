GO_LIBRARY()

SRCS(
    global.go
    grpc_request.go
    http_call.go
    logging.go
    scope.go
)

GO_TEST_SRCS(
    common_test.go
    global_test.go
    grpc_request_test.go
    http_call_test.go
)

END()

RECURSE(gotest)
