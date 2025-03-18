GO_LIBRARY()

OWNER(g:go-library)

RESOURCE(
    certs/localhost.crt certs/localhost.crt
    certs/localhost.key certs/localhost.key
)

SRCS(
    interceptor_suite.go
    pingservice.go
)

END()

RECURSE(testproto)
