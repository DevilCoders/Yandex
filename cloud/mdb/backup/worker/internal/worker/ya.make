GO_LIBRARY()

OWNER(g:mdb)

SRCS(worker.go)

GO_TEST_SRCS(worker_test.go)

END()

RECURSE(gotest)
