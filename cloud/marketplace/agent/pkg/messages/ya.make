GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(messages.go)

GO_TEST_SRCS(messages_test.go)

END()

RECURSE(gotest)
