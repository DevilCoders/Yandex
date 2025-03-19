OWNER(g:cloud-iam)

GO_LIBRARY()

SRCS()

NO_LINT()

FROM_SANDBOX(FILE 3294431038 OUT_NOAUTO fixture_permissions.yaml)

RUN_PROGRAM(
    cloud/iam/codegen/python/codegen
    -m cloud/iam/codegen/go/resources/fixture_permissions.yaml
    -t cloud/iam/codegen/go/resources/resources.go.tmpl
    -o resources.go
    IN cloud/iam/codegen/go/resources/fixture_permissions.yaml
    IN cloud/iam/codegen/go/resources/resources.go.tmpl
    OUT resources.go
)

GO_TEST_SRCS(
    resources_test.go
)

END()

RECURSE_FOR_TESTS(gotest)
