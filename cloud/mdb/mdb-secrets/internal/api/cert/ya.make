GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    delete.go
    get.go
    put.go
    service.go
)

GO_TEST_SRCS(
    delete_test.go
    get_test.go
    put_test.go
)

END()

RECURSE(gotest)
