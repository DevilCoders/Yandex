GO_LIBRARY()

OWNER(g:mdb)

SRCS(accessservice.go)

END()

RECURSE(
    grpc
    mocks
    stub
)
