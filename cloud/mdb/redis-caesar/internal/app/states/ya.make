GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    candidate.go
    first_run.go
    lost.go
    manager.go
    states.go
)

GO_TEST_SRCS(states_test.go)

END()

RECURSE(
    gotest
    mock
)
