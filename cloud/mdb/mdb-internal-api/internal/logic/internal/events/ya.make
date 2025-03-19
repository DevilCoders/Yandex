GO_LIBRARY()

OWNER(g:mdb)

SRCS(events.go)

END()

RECURSE(
    mocks
    provider
)
