GO_LIBRARY()

OWNER(g:mdb)

SRCS(crypto.go)

END()

RECURSE(
    mocks
    nacl
)
