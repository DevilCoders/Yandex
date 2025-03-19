GO_LIBRARY()

OWNER(g:mdb)

SRCS(backup.go)

GO_TEST_SRCS(backup_test.go)

END()

RECURSE(gotest)
