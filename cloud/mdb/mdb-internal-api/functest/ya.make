GO_LIBRARY()

OWNER(g:mdb)

DATA(arcadia/cloud/mdb/salt/pillar/metadb_default_versions.sls)

SRCS(
    commonsteps.go
    datacloudsteps.go
    functest.go
    grpccontext.go
    grpcsteps.go
    helpers.go
    iammockdata.go
    licensemock.go
    logsdbcontext.go
    logsdbsteps.go
    metadbsteps.go
    models.go
    mongoperfdiagmockdata.go
    myperfdiagmockdata.go
    pgperfdiagmockdata.go
    reindxersteps.go
    restcontext.go
    reststeps.go
    stephelpers.go
    testcontext.go
)

GO_TEST_SRCS(stephelpers_test.go)

END()

RECURSE(
    gotest
    tests
)
