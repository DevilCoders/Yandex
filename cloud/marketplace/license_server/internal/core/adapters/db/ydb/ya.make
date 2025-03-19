GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    errors.go
    license_instances.go
    license_locks.go
    license_server.go
    license_template_versions.go
    license_templates.go
    operations.go
    transforms.go
)

END()

RECURSE(gotest)
