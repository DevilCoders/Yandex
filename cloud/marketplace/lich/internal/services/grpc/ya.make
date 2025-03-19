GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    base.go
    configurator.go
    errors.go
    license_check_service.go
    service.go
)

END()

RECURSE(
    interceptors
)
