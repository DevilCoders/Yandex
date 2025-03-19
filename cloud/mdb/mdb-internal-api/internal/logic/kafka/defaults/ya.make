GO_LIBRARY()

OWNER(g:mdb)

SRCS(defaults.go)

GO_TEST_SRCS(defaults_test.go)

END()

RECURSE(gotest)
