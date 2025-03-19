GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    auth.go
    auth_test_plugs.go
    backups.go
    cluster.go
    editions.go
    extension.go
    operation.go
    permissions.go
    plugins.go
    saml_settings.go
    tasks.go
    user.go
    versions.go
)

GO_TEST_SRCS(
    auth_test.go
    cluster_test.go
    saml_settings_test.go
    versions_test.go
)

END()

RECURSE(gotest)
