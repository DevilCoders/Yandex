GO_LIBRARY()

OWNER(g:mdb)

SRCS(date.go)

GO_XTEST_SRCS(date_test.go)

END()

RECURSE(gotest)
