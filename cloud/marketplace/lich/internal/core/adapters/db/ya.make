GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    interfaces.go
)

END()

RECURSE(
    ydb
)
