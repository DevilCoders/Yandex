GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    root.go
    completion.go
)

END()

RECURSE(
    blueprints
    builds
    console
    operations
    products
    profile
    publishers
    yaga
)
