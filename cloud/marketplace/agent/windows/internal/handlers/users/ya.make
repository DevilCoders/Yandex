GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    users_handler.go
    users_request.go
    users_response.go
)

GO_TEST_SRCS(users_handler_test.go)

END()

RECURSE(gotest)
