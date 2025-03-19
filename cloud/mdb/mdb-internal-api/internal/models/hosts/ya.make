GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    health.go
    host.go
    role.go
)

END()

RECURSE(
    services
    system
)
