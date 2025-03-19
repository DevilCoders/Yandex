GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    app.go
    config.go
    options.go
    run.go
)

GO_TEST_SRCS(app_test.go)

END()

RECURSE(
    gotest
    metadb
)
