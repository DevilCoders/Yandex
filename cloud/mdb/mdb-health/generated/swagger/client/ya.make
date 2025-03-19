GO_LIBRARY()

OWNER(g:mdb)

SRCS(mdb_health_client.go)

END()

RECURSE(
    clusterhealth
    health
    hostneighbours
    hostshealth
    listhostshealth
    unhealthyaggregatedinfo
)
