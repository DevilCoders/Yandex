GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(base_action.go)

END()

RECURSE(
    errors
    license
    operations
)
