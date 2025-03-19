GO_LIBRARY()

OWNER(g:mdb)

SRCS(tracing.go)

END()

RECURSE(
    jaeger
    tags
)
