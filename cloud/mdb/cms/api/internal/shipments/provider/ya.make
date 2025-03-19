GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    deploy.go
    ensure_no_primary.go
)

GO_TEST_SRCS(ensure_no_primary_test.go)

END()

RECURSE_FOR_TESTS(gotest)
