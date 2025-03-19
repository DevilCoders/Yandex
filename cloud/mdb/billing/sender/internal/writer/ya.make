GO_LIBRARY()

OWNER(g:mdb)

SRCS(writer.go)

END()

RECURSE(
    kinesis
    logbroker
    mocks
)
