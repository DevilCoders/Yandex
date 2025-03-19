GO_LIBRARY()

OWNER(g:mdb)

SRCS(auth.go)

END()

RECURSE(
    accessservice
    mocks
)
