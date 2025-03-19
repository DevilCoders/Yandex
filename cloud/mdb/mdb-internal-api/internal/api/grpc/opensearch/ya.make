GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    auth_service.go
    backup_service.go
    cluster_service.go
    console_cluster_service.go
    extension_service.go
    models.go
    operation_service.go
    resource_preset_service.go
)

GO_TEST_SRCS(models_test.go)

END()

RECURSE(gotest)
