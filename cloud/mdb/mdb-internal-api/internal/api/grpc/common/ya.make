GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    consoleservice.go
    healthservice.go
    operationservice.go
    quotaservice.go
)

GO_TEST_SRCS(
    consoleservice_test.go
    quotaservice_test.go
)

END()

RECURSE(gotest)
