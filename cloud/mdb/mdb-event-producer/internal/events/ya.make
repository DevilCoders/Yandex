GO_LIBRARY()

OWNER(g:mdb)

SRCS(events.go)

GO_XTEST_SRCS(events_test.go)

END()

RECURSE(gotest)
