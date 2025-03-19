GO_LIBRARY()

OWNER(g:mdb)

SRCS(backend.go)

GO_TEST_SRCS(backend_test.go)

END()

RECURSE(gotest)
