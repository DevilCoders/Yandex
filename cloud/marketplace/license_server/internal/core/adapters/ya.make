GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(errors.go)

END()

RECURSE(
    billing
    db
    gotest
    marketplace
    mocks
)
