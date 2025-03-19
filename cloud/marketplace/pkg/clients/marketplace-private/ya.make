GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    categories.go
    client.go
    errors.go
    interfaces.go
    operations.go
    products.go
    publishers.go
    session.go
    tariffs.go
)

END()

RECURSE(dev)
