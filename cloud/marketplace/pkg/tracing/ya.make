GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    config.go
    default.go
    tracing.go
)

END()

RECURSE(dev)
