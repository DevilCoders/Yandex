GO_LIBRARY()

OWNER(g:mdb)

SRCS(producer.go)

GO_XTEST_SRCS(producer_test.go)

END()

RECURSE(gotest)
