GO_LIBRARY()

SRCS(
    e2e.go
)

GO_TEST_SRCS(
    e2e_test.go
)

END()

RECURSE(
    gotest
)
