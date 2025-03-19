GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    errors.go
    interface.go
)

END()

RECURSE(
    mlock
    mocks
)
