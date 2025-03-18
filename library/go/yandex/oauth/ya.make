GO_LIBRARY()

OWNER(
    buglloc
    g:go-library
)

SRCS(
    errors.go
    ssh.go
    ssh_keyring.go
    ssh_options.go
)

GO_TEST_SRCS(export_ssh_options_test.go)

GO_XTEST_SRCS(
    ssh_example_test.go
    ssh_keyring_test.go
    ssh_test.go
)

END()

RECURSE(gotest)
