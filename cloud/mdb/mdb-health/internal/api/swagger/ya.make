GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    api.go
    errors.go
)

GO_TEST_SRCS(api_test.go)

END()

RECURSE(gotest)
