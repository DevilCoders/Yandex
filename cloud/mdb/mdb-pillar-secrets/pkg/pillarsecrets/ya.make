GO_LIBRARY()

OWNER(g:mdb)

SRCS(pillarsecrets.go)

END()

RECURSE(
    grpc
    mocks
)
