GO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    copy.go
    delete.go
    move.go
    tasks.go
)

GO_TEST_SRCS(tasks_test.go)

END()

RECURSE(gotest)
