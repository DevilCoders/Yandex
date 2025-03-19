GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    failoverminions.go
    knownmaster.go
    service.go
    syncmasterslist.go
    timeouts.go
)

GO_XTEST_SRCS(service_test.go)

END()

RECURSE(gotest)
