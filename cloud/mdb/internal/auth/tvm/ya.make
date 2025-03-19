GO_LIBRARY()

OWNER(g:mdb)

SRCS(tvm.go)

GO_XTEST_SRCS(tvm_test.go)

END()

RECURSE(
    gotest
    tvmtool
)
