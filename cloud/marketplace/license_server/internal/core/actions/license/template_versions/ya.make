GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    apply.go
    create.go
    delete.go
    get.go
    list_by_template_id.go
    update.go
)

GO_TEST_SRCS(
    apply_test.go
    create_test.go
    delete_test.go
    get_test.go
    update_test.go
)

END()

RECURSE(gotest)
