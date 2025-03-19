GO_LIBRARY()

OWNER(
    g:mdb-dataproc
    g:mdb
)

SRCS(user_auth.go)

GO_TEST_SRCS(
    integration_test.go
    user_auth_test.go
)

END()

RECURSE(
    gotest
    mocks
)
