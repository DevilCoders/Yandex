GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    dns_client.go
    live_by_cluster_parameters.go
    live_by_cluster_responses.go
    live_parameters.go
    live_responses.go
    ping_parameters.go
    ping_responses.go
    stats_parameters.go
    stats_responses.go
    update_primary_dns_parameters.go
    update_primary_dns_responses.go
)

END()
