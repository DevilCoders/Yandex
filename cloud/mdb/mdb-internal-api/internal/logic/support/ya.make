GO_LIBRARY()

OWNER(g:mdb)

SRCS(support.go)

END()

RECURSE(
    clmodels
    provider
)
