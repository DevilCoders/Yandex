GO_LIBRARY()

OWNER(g:mdb)

SRCS(queueproducer.go)

END()

RECURSE(
    mocks
    provider
)
