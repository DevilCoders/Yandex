GO_LIBRARY()

OWNER(g:mdb)

SRCS(internalapi.go)

END()

RECURSE(
    grpc
    mocks
)
