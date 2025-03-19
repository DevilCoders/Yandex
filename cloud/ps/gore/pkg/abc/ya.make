GO_LIBRARY()

OWNER(g:cloud-ps)

SRCS(abc.go)

END()

RECURSE(
    http
    mocks
)
