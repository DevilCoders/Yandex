GO_LIBRARY()

OWNER(g:mdb)

SRCS(jobproducer.go)

GO_TEST_SRCS(jobproducer_test.go)

END()

RECURSE(gotest)
