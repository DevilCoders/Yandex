GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    api.go
    config.go
    gw.go
    lead_loop.go
    process_sli.go
    stat.go
    topology.go
)

GO_TEST_SRCS(process_sli_test.go)

GO_XTEST_SRCS(api_test.go)

END()

RECURSE(
    gotest
    types
)
