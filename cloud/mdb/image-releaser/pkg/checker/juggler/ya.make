GO_LIBRARY()

OWNER(g:mdb)

SRCS(juggler.go)

GO_TEST_SRCS(juggler_test.go)

END()

RECURSE(gotest)
