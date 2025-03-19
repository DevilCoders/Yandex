GO_LIBRARY()

OWNER(g:cloud-billing)

SRCS(
    common_entities.go
    enum.go
    enums_string.go
    reflect.go
    sku.go
    types.go
    validate.go
)

GO_TEST_SRCS(
    common_entities_test.go
    sku_test.go
)

END()

RECURSE(gotest)
