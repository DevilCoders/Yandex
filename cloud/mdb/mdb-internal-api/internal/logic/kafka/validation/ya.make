GO_LIBRARY()

OWNER(g:mdb)

SRCS(config.go)

GO_TEST_SRCS(config_test.go)

END()

RECURSE(gotest)
