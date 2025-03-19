GO_LIBRARY()

OWNER(g:mdb)

SRCS(provider.go)

GO_TEST_SRCS(provider_test.go)

END()

RECURSE(gotest)
