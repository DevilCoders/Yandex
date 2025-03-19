GO_LIBRARY()

OWNER(g:mdb)

SRCS(katandb.go)

END()

RECURSE(
    mocks
    pg
)
