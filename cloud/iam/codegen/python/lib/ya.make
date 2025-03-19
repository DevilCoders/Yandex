OWNER(g:cloud-iam)

PY23_LIBRARY(yc_constants)

FROM_SANDBOX(FILE 1732716983 OUT_NOAUTO fixture_permissions.yaml)

PY_SRCS(
    NAMESPACE yc_constants
    permissions.py
    resources.py
    quotas.py
)

RUN_PROGRAM(
    cloud/iam/codegen/python/codegen
    -m cloud/iam/codegen/python/lib/fixture_permissions.yaml
    -t cloud/iam/codegen/python/codegen/permissions.python.tmpl
    -o permissions.py
    IN cloud/iam/codegen/python/lib/fixture_permissions.yaml
    IN cloud/iam/codegen/python/codegen/permissions.python.tmpl
    OUT_NOAUTO permissions.py
)

RUN_PROGRAM(
    cloud/iam/codegen/python/codegen
    -m cloud/iam/codegen/python/lib/fixture_permissions.yaml
    -t cloud/iam/codegen/python/codegen/resources.python.tmpl
    -o resources.py
    IN cloud/iam/codegen/python/lib/fixture_permissions.yaml
    IN cloud/iam/codegen/python/codegen/resources.python.tmpl
    OUT_NOAUTO resources.py
)

RUN_PROGRAM(
    cloud/iam/codegen/python/codegen
    -m cloud/iam/codegen/python/lib/fixture_permissions.yaml
    -t cloud/iam/codegen/python/codegen/quotas.python.tmpl
    -o quotas.py
    IN cloud/iam/codegen/python/lib/fixture_permissions.yaml
    IN cloud/iam/codegen/python/codegen/quotas.python.tmpl
    OUT_NOAUTO quotas.py
)

END()

RECURSE_FOR_TESTS(
    ut
)
