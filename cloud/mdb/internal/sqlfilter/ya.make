GO_LIBRARY()

OWNER(g:mdb)

SRCS(filters.go)

GO_XTEST_SRCS(filters_test.go)

END()

RECURSE(
    chencoder
    gotest
    grammar
)
