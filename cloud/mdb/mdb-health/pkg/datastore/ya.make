GO_LIBRARY()

OWNER(g:mdb)

SRCS(datastore.go)

END()

RECURSE(
    mocks
    redis
)
