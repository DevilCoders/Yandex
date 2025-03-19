GO_LIBRARY()

OWNER(g:mdb)

SRCS(cluster_discovery.go)

END()

RECURSE(
    metadbdiscovery
    mocks
)
