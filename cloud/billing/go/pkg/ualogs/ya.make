GO_LIBRARY()

OWNER(g:cloud-billing)

SRCS(
    register.go
    sink.go
)

GO_TEST_SRCS(sink_test.go)

END()

RECURSE(gotest)
