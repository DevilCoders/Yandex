GO_LIBRARY()

SRCS(
    grpc.go
    interface.go
    ydb.go
)

END()

RECURSE(
    cloudjwt
    cloudmeta
    tvmticket
)
