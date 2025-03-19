GO_LIBRARY()

OWNER(g:mdb)

SRCS(app.go)

GO_TEST_SRCS(app_test.go)

END()

RECURSE(
    gotest
    states
)
