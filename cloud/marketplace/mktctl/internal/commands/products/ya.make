GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    fcreate.go
    get.go
    lifecycle.go
    list.go
    root.go
)

END()

RECURSE(
    proposals
    versions
)
