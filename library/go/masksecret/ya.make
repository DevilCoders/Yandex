GO_LIBRARY()

OWNER(
    gzuykov
    g:go-library
)

SRCS(
    credit_card.go
    email.go
    http_request.go
    phone.go
    secret.go
    string.go
    url.go
)

GO_TEST_SRCS(
    credit_card_test.go
    email_test.go
    http_request_test.go
    phone_test.go
    secret_test.go
    string_test.go
    url_test.go
)

END()

RECURSE(gotest)
