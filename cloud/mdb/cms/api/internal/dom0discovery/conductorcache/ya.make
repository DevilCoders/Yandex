GO_LIBRARY()

OWNER(g:mdb)

SRCS(cache.go)

GO_XTEST_SRCS(cache_test.go)

END()

RECURSE(gotest)
