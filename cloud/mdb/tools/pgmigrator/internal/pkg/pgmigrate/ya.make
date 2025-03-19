GO_LIBRARY()

OWNER(g:mdb)

SRCS(pgmigrate.go)

GO_TEST_SRCS(pgmigrate_test.go)

END()

RECURSE(gotest)
