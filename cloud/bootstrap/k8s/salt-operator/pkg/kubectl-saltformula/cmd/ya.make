GO_LIBRARY()

SRCS(cmd.go)

GO_TEST_SRCS(cmd_test.go)

END()

RECURSE(
    get
    gotest
)
