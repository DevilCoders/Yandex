GO_LIBRARY()

OWNER(
    buglloc
    g:go-library
)

SRCS(
    client.go
    client_opts.go
    http.go
)

GO_XTEST_SRCS(
    client_checkip_test.go
    client_example_test.go
    client_leak_test.go
    client_multisessionid_test.go
    client_oauth_test.go
    client_retries_test.go
    client_sessionid_test.go
    client_srv_test.go
    client_tvm_test.go
    client_userinfo_test.go
    client_userticket_test.go
)

END()

RECURSE(
    bbtypes
    examples
    gotest
)
