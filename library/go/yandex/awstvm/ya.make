GO_LIBRARY()

OWNER(
    buglloc
    g:go-library
)

SRCS(
    auth_provider.go
    doc.go
    options.go
    s3.go
    s3_auth_provider.go
    sqs.go
)

GO_XTEST_SRCS(
    auth_provider_test.go
    mocktvm_test.go
    s3_example_test.go
    s3_test.go
    sqs_example_test.go
    sqs_test.go
)

END()

RECURSE(gotest)
