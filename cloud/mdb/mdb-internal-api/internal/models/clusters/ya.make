GO_LIBRARY()

OWNER(g:mdb)

REQUIREMENTS(ram:11)

SRCS(
    access.go
    cluster.go
    encryption.go
    health.go
    labels.go
    maintenance.go
    roles.go
    status.go
    type.go
    versions.go
)

GO_TEST_SRCS(
    labels_test.go
    maintenance_test.go
)

END()

RECURSE(gotest)
