GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    alert.go
    config.go
)

GO_TEST_SRCS(alert_test.go)

END()

RECURSE(gotest)
