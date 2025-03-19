GO_LIBRARY()

OWNER(g:mdb)

SRCS(migrator.go)

GO_TEST_SRCS(migrator_test.go)

END()

RECURSE(gotest)
