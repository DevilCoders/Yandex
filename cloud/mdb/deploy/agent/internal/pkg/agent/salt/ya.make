GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    command.go
    conflict.go
    job.go
)

GO_TEST_SRCS(command_test.go)

GO_XTEST_SRCS(conflict_test.go)

END()

RECURSE(gotest)
