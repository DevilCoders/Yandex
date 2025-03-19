GO_LIBRARY()

OWNER(g:mdb)

SRCS(tasks.go)

END()

RECURSE(
    mocks
    provider
)
