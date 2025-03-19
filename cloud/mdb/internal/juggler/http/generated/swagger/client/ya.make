GO_LIBRARY()

OWNER(g:mdb)

SRCS(juggler_client.go)

END()

RECURSE(
    checks
    dashboards
    downtimes
    internal_swagger
    mutes
    namespaces
    notifications
    operations
    raw_events
)
