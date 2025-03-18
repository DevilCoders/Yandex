GO_LIBRARY()

OWNER(
    g:go-library
    prime
)

SRCS(
    client.go
    error.go
    instance_spec.go
    model.go
    resources.go
)

END()

RECURSE(
    gotest
    httpnanny
)
