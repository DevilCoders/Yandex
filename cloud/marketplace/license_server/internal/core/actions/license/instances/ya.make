GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    activate_pending.go
    cancel.go
    create.go
    delete.go
    deprecate.go
    get.go
    list.go
    recreate.go
)

GO_TEST_SRCS(
    cancel_test.go
    create_test.go
    delete_test.go
    deprecate_test.go
    get_test.go
    list_test.go
)

END()

RECURSE(gotest)
