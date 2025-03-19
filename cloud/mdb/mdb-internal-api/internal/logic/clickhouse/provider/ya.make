GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    backups.go
    clickhouse.go
    clusters.go
    console.go
    create.go
    databases.go
    datacloud_modify.go
    dictionaries.go
    formatschemas.go
    hosts.go
    mdb_modify.go
    mlmodels.go
    restore.go
    shard_groups.go
    shards.go
    users.go
    utils.go
)

GO_TEST_SRCS(
    clickhouse_test.go
    databases_test.go
    datacloud_modify_test.go
    restore_test.go
    shard_groups_test.go
    shards_test.go
    utils_test.go
)

END()

RECURSE(
    gotest
    internal
)
