GO_LIBRARY()

OWNER(g:mdb)

SRCS(queuehandler.go)

END()

RECURSE(
    mocks
    provider
)
