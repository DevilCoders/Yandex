GO_LIBRARY()

OWNER(g:mdb)

SRCS(locker.go)

END()

RECURSE(
    mlock
    mocks
)
