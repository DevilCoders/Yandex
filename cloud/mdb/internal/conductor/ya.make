GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    conductor.go
    helpers.go
)

GO_XTEST_SRCS(helpers_test.go)

END()

RECURSE(
    gotest
    httpapi
    mocks
)
