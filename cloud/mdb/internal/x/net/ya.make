GO_LIBRARY()

OWNER(g:mdb)

SRCS(addr.go)

GO_XTEST_SRCS(addr_test.go)

END()

RECURSE(
    dial
    gotest
    url
)
