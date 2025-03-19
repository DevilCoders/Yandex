GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    cluster.go
    operations.go
    permissions.go
    resource_types.go
    valid.go
)

END()

RECURSE(
    backups
    clusters
    console
    environment
    hosts
    kafka
    logs
    monitoring
    operations
    optional
    pagination
    pillars
    quota
    resources
    tasks
)
