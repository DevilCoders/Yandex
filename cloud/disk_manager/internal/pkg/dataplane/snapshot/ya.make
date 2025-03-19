OWNER(g:cloud-nbs)

GO_LIBRARY()

SRCS(
    source.go
    target.go
)

END()

RECURSE(
    config
    storage
)
