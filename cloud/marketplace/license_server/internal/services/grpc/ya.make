GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    configurator.go
    service.go
)

END()

RECURSE(
    base
    errors
    interceptors
    license
    operations
)
