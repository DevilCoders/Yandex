GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    backups.go
    clusters.go
    hosts.go
    redis.go
    shards.go
    utils.go
)

END()

RECURSE(internal)
