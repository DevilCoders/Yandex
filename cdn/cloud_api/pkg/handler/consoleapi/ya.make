GO_LIBRARY()

OWNER(
    g:strm-admin
    g:traffic
)

SRCS(
    cache.go
    operation.go
    origin.go
    origins_group.go
    provider.go
    raw_logs.go
    resource.go
    resource_mapper.go
    resource_metrics.go
    resource_rules.go
    utils.go
)

END()
