OWNER(g:cloud-iam)

GO_LIBRARY()

SRCS()

NO_LINT()

FROM_SANDBOX(FILE 3294431038 OUT_NOAUTO fixture_permissions.yaml)

RUN_PROGRAM(
    cloud/iam/codegen/python/codegen
    -m cloud/iam/codegen/go/quotas/fixture_permissions.yaml
    -t cloud/iam/codegen/go/quotas/quotas.go.tmpl
    -o quotas.go
    IN cloud/iam/codegen/go/quotas/fixture_permissions.yaml
    IN cloud/iam/codegen/go/quotas/quotas.go.tmpl
    OUT quotas.go
)

GO_TEST_SRCS(
    quotas_test.go
)

END()

RECURSE_FOR_TESTS(gotest)
