GO_LIBRARY()

OWNER(g:mdb)

SRCS(notifier.go)

END()

RECURSE(
    http
    mocks
    nop
)
