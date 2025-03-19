GO_LIBRARY()

OWNER(g:mdb)

SRCS(address.go)

GO_XTEST_SRCS(address_test.go)

END()

RECURSE(gotest)
