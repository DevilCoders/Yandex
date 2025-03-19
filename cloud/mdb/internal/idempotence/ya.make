GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    idempotence.go
    incoming.go
    outgoing.go
)

GO_XTEST_SRCS(
    incoming_test.go
    outgoing_test.go
)

END()

RECURSE(gotest)
