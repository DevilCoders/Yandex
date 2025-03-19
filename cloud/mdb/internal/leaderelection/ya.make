GO_LIBRARY()

OWNER(g:mdb)

SRCS(leader_elector.go)

END()

RECURSE(
    mocks
    rediscachetier
    redsync
)
