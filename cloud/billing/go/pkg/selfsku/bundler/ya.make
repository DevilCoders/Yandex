GO_LIBRARY()

OWNER(g:cloud-billing)

SRCS(
    core.go
    load.go
    migrate.go
    output.go
    types.go
)

END()

RECURSE(migrator)
