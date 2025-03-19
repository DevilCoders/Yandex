GO_LIBRARY()

OWNER(g:mdb)

SRCS(vpc.go)

END()

RECURSE(
    grpc
    mocks
    nop
)
