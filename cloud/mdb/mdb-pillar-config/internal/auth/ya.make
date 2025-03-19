GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    auth.go
    context.go
)

END()

RECURSE(
    mocks
    nop
    provider
)
