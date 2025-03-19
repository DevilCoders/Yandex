OWNER(g:cloud-iam)

GO_LIBRARY()

SRCS()

FROM_SANDBOX(FILE 3294431038 OUT_NOAUTO fixture_permissions.yaml)

RUN_PROGRAM(
    cloud/iam/codegen/python/codegen
    -m cloud/iam/codegen/go/permissions/fixture_permissions.yaml
    -t cloud/iam/codegen/go/permissions/permissions.go.tmpl
    -o permissions.go
    IN cloud/iam/codegen/go/permissions/fixture_permissions.yaml
    IN cloud/iam/codegen/go/permissions/permissions.go.tmpl
    OUT permissions.go
)

GO_TEST_SRCS(
    permissions_test.go
)

END()

RECURSE_FOR_TESTS(gotest)
