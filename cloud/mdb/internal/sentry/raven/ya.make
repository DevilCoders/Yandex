GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    client.go
    stacktrace.go
)

GO_TEST_SRCS(stacktrace_test.go)

END()

RECURSE(gotest)
