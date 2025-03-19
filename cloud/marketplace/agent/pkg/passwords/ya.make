GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(passwords.go)

GO_TEST_SRCS(passwords_test.go)

END()

RECURSE(
    gotest
    mock
)
