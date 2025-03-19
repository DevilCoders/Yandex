GO_LIBRARY()

OWNER(g:mdb)

SRCS(queueproducer.go)

GO_TEST_SRCS(queueproducer_test.go)

END()

RECURSE(gotest)
