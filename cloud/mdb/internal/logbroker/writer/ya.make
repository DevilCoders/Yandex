GO_LIBRARY()

OWNER(g:mdb)

SRCS(writer.go)

GO_XTEST_SRCS(writer_test.go)

END()

RECURSE(
    dummy
    gotest
    logbroker
    mocks
)
