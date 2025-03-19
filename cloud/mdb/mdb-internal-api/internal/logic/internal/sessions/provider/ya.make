GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    marker.go
    sessions.go
)

GO_TEST_SRCS(sessions_test.go)

END()

RECURSE(gotest)
