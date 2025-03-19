GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    clusters.go
    connectors.go
    console.go
    create.go
    kafka.go
    modify.go
    search.go
    topics.go
    users.go
    utils.go
)

GO_TEST_SRCS(
    clusters_test.go
    connectors_test.go
    console_test.go
    create_test.go
    kafka_test.go
    modify_test.go
    users_test.go
    utils_test.go
)

END()

RECURSE(
    gotest
    internal
)
