GO_LIBRARY()

OWNER(g:mdb)

SRCS(types.go)

GO_XTEST_SRCS(types_test.go)

END()

RECURSE(
    gotest
    testhelpers
)
