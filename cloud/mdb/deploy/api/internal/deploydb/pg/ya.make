GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    cleanup.go
    commands.go
    config.go
    errors.go
    groups.go
    jobresults.go
    jobs.go
    masters.go
    minions.go
    models.go
    pg.go
    shipments.go
)

GO_XTEST_SRCS(pg_integration_test.go)

END()

RECURSE(gotest)
