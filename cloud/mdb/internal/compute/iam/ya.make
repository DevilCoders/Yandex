GO_LIBRARY()

OWNER(g:mdb)

SRCS(iam.go)

END()

RECURSE(
    cmd
    grpc
    internal
    mocks
    nop
)
