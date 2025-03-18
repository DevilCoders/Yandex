GO_LIBRARY()

OWNER(
    g:strm-admin
    g:traffic
)

SRCS(
    operation_name.go
    request_id.go
)

END()

RECURSE(
    grpc
    http
    metrics
)
