GO_LIBRARY()

OWNER(g:go-library)

SRCS(stacktrace.go)

GO_TEST_SRCS(stacktrace_benchmark_test.go)

END()

RECURSE(gotest)
