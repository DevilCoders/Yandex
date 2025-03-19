GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    apply.go
    delete.go
    get.go
    list_by_template_id.go
    update.go
    versions.go
)

GO_TEST_SRCS(delete_test.go)

END()

RECURSE(gotest)
