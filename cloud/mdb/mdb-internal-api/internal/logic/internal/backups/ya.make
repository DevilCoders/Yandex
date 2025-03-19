GO_LIBRARY()

OWNER(g:mdb)

SRCS(backups.go)

END()

RECURSE(
    mocks
    provider
)
