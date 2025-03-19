GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    cluster.go
    config.go
    databases.go
    hosts.go
    operation.go
    restore_hints.go
    tasks.go
    users.go
)

GO_TEST_SRCS(config_test.go)

END()

RECURSE(gotest)
