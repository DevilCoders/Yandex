GO_TEST()

OWNER(g:cloud-nbs)

SRCS(
    test.go
)

DEPENDS(
    cloud/storage/core/tools/ops/config_generator
)

END()
