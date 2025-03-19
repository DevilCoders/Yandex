GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    return.go
    return_states.go
    saltapi.go
)

GO_TEST_SRCS(
    return_states_test.go
    return_test.go
)

END()

RECURSE(
    cherrypy
    configs
    gotest
    mocks
    tracing
)
