GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    backups.go
    clusters.go
    databases.go
    hosts.go
    modify.go
    s3.go
    search.go
    sqlserver.go
    users.go
)

END()

RECURSE(internal)
