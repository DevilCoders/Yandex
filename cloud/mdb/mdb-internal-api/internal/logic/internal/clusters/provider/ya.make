GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    billing.go
    clusters.go
    hostname.go
    hosts.go
    maintenance.go
    misc.go
    operate.go
    pillars.go
    placement_group.go
    provider.go
    resources.go
    secrets.go
    shards.go
    subclusters.go
)

GO_TEST_SRCS(
    hostname_test.go
    maintenance_test.go
    operate_test.go
    resources_test.go
)

END()

RECURSE(
    gotest
    hostname
    mocks
    rsa
)
