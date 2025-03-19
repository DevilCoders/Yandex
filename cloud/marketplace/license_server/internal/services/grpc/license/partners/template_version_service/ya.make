GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    create.go
    get.go
    list_by_template_id.go
    update.go
    versions.go
)

GO_TEST_SRCS(create_test.go)

END()

RECURSE(gotest)
