GO_LIBRARY()

OWNER(g:mdb)

SRCS(sender.go)

GO_TEST_SRCS(sender_test.go)

END()

RECURSE(gotest)
