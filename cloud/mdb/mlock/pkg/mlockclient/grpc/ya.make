GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    client.go
    create_lock.go
    deserialize.go
    get_lock_status.go
    release_lock.go
)

GO_TEST_SRCS(client_test.go)

END()

RECURSE(gotest)
