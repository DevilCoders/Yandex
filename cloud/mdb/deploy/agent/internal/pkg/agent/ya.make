GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    agent.go
    deps.go
    metrics.go
)

GO_XTEST_SRCS(agent_test.go)

END()

RECURSE(
    call
    gotest
    mocks
    salt
    srv
)
