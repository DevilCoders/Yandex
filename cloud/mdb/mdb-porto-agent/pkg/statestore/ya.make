GO_LIBRARY()

OWNER(g:mdb)

SRCS(statestore.go)

END()

RECURSE(
    fs
    mocks
)
