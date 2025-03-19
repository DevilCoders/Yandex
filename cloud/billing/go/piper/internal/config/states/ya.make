GO_LIBRARY()

SRCS(
    http.go
    manager.go
    reflect.go
    shutdown.go
)

GO_TEST_SRCS(
    manager_test.go
    reflect_test.go
)

END()

RECURSE(gotest)
