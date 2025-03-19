GO_LIBRARY()

OWNER(g:mdb)

SRCS(monrun_interactor.go)

GO_TEST_SRCS(monrun_interactor_test.go)

END()

RECURSE(
    alerting
    authorization
    autoduty
    gotest
    metrics
    mwswitch
    walle
)
