GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    0000_migration.go
    0001_license_templates.go
    0002_license_template_versions.go
    0003_license_instances.go
    0004_license_locks.go
    0005_operations.go
    migrations.go
)

END()

RECURSE(gotest)
