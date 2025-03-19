GO_LIBRARY()

OWNER(g:mdb)

SRCS(juggler.go)

END()

RECURSE(
    http
    mocks
)
