GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    backup.go
    cluster.go
    config.go
    console.go
    constants.go
    database.go
    dictionary.go
    formatschema.go
    host.go
    mlmodel.go
    operation.go
    resourcepreset.go
    shard.go
    shard_group.go
    tasks.go
    user.go
    version.go
)

GO_TEST_SRCS(
    config_test.go
    user_test.go
)

END()

RECURSE(gotest)
