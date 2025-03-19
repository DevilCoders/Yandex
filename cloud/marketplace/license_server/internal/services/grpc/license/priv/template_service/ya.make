GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    delete.go
    deprecate.go
    get.go
    list_by_tariff_id.go
    templates.go
)

GO_TEST_SRCS(
    delete_test.go
    deprecate_test.go
    get_test.go
)

END()

RECURSE(gotest)
