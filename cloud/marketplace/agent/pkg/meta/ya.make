GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    eventbus.go
    poller.go
)

GO_TEST_SRCS(
    eventbus_test.go
    poller_test.go
)

END()

RECURSE(gotest)
