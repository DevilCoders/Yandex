GO_LIBRARY()

OWNER(g:mdb)

SRCS(deployutils.go)

GO_TEST_SRCS(deployutils_test.go)

END()

RECURSE(gotest)
