GO_LIBRARY()

OWNER(g:mdb)

SRCS(context.go)

GO_XTEST_SRCS(context_test.go)

END()

RECURSE(
    blackbox
    gotest
    grpcauth
    httpauth
    tvm
)
