GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    cluster.go
    config.go
    connector.go
    constants.go
    operation.go
    permissions.go
    tasks.go
    topic.go
    user.go
    version.go
)

GO_TEST_SRCS(
    config_test.go
    connector_test.go
    topic_test.go
    user_test.go
    version_test.go
)

END()

RECURSE(gotest)
