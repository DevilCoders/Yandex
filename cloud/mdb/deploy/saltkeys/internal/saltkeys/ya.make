GO_LIBRARY()

OWNER(g:mdb)

SRCS(saltkeys.go)

GO_XTEST_SRCS(saltkeys_test.go)

END()

RECURSE(
    gotest
    mocks
    skcli
)
