GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    health.go
    logs.go
    operations.go
    quotas.go
)

GO_TEST_SRCS(logs_test.go)

END()

RECURSE(gotest)
