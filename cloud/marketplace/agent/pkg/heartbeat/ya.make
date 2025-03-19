GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(heartbeat.go)

GO_TEST_SRCS(heartbeat_test.go)

END()

RECURSE(gotest)
