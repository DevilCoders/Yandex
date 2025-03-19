GO_LIBRARY()

OWNER(g:mdb)

SRCS(redis.go)

END()

RECURSE(
    mocks
    provider
    rmodels
)
