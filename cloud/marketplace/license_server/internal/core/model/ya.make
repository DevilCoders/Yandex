GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    operation.go
    period.go
    price.go
    sku.go
    tariff.go
)

END()

RECURSE(license)
