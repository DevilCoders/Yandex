GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    license_instances.go
    license_locks.go
    license_server.go
    license_template_versions.go
    license_templates.go
    operations.go
)

GO_TEST_SRCS(
    base_test.go
    license_instances_test.go
    license_locks_test.go
    license_template_versions_test.go
    license_templates_test.go
    operations_test.go
)

END()

RECURSE(
    errors
    gotest
    migrations
)
