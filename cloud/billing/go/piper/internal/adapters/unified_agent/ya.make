GO_LIBRARY()

SRCS(
    adapter.go
    push.go
)

GO_TEST_SRCS(
    common_test.go
    push_test.go
)

END()

RECURSE(gotest)
