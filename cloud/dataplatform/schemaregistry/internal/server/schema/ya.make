GO_LIBRARY()

OWNER(tserakhau)

SRCS(
    compatibility.go
    models.go
    service.go
)

GO_XTEST_SRCS(service_test.go)

END()

RECURSE(
    gotest
    provider
)
