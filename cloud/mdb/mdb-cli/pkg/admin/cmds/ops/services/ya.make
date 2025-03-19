GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    controlplane.go
    get.go
    helpers.go
    list.go
    services.go
    test.go
    upgrade.go
)

GO_TEST_SRCS(upgrade_test.go)

GO_EMBED_PATTERN(templates/*)

END()

RECURSE(gotest)
