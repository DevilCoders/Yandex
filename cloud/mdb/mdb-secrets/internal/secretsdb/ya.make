GO_LIBRARY()

OWNER(g:mdb)

SRCS(service.go)

END()

RECURSE(
    mocks
    pg
)
