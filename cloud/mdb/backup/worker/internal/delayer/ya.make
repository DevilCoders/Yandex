GO_LIBRARY()

OWNER(g:mdb)

SRCS(fractiondelayer.go)

GO_TEST_SRCS(fractiondelayer_test.go)

END()

RECURSE(gotest)
