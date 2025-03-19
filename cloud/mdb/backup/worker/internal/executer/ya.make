GO_LIBRARY()

OWNER(g:mdb)

SRCS(executer.go)

END()

RECURSE(
    mocks
    provider
)
