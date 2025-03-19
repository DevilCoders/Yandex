GO_LIBRARY()

OWNER(g:mdb)

SRCS(marketplace.go)

END()

RECURSE(
    grpc
    http
    mocks
    stub
)
