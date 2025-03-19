GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    app_dcs.go
    app.go
    cli.go
    data.go
    replication.go
    util.go
)

GO_TEST_SRCS(util_test.go)

END()

RECURSE(gotest)
