GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    backups.go
    clusters.go
    databases.go
    hosts.go
    mongodb.go
    perfdiag.go
    users.go
)

END()

RECURSE(internal)
