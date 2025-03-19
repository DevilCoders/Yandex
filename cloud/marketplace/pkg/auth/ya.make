GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    interfaces.go
    token.go
)

END()

RECURSE(
    access-backend
    dummy-backend
    permissions
)
