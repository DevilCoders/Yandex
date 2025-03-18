GO_LIBRARY()

OWNER(
    g:go-library
    g:solomon
)

SRCS(
    admin.go
    admin_options.go
    error.go
    model.go
    options.go
)

END()

RECURSE(
    admin
    reporters
)
