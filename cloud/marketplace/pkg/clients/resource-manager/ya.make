GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    client.go
    cloud_service.go
    errors.go
)

END()

RECURSE(dev)
