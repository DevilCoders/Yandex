GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    generate.go
    health.go
    logs.go
    operation_util.go
    operations.go
    quotas.go
)

END()

RECURSE(
    mocks
    provider
)
