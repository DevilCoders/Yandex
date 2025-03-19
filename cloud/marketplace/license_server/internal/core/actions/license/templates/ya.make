GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    create.go
    delete.go
    deprecate.go
    get.go
    list_by_tariff_id.go
)

GO_TEST_SRCS(
    create_test.go
    delete_test.go
    deprecate_test.go
    get_test.go
)

END()

RECURSE(gotest)
