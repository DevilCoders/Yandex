GO_LIBRARY()

OWNER(g:mdb)

SRCS(health.go)

GO_TEST_SRCS(health_test.go)

END()

RECURSE(
    flaps
    gotest
)
