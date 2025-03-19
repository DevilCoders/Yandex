GO_LIBRARY()

OWNER(g:mdb)

SRCS(compute.go)

END()

RECURSE(
    accessservice
    billing
    compute
    dns
    iam
    marketplace
    operations
    resmanager
    vpc
)
