GO_LIBRARY()

OWNER(
    g:strm-admin
    g:traffic
)

SRCS(
    core.go
    logger.go
    origin.go
    origins_group.go
    resource.go
    resource_rule.go
    utils.go
)

END()
