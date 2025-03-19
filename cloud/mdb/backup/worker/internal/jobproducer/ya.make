GO_LIBRARY()

OWNER(g:mdb)

SRCS(jobproducer.go)

END()

RECURSE(
    mocks
    provider
)
