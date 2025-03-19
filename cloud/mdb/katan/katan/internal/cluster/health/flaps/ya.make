GO_LIBRARY()

OWNER(g:mdb)

SRCS(flaps.go)

GO_TEST_SRCS(flaps_test.go)

END()

RECURSE(gotest)
