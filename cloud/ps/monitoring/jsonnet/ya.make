EXECTEST()
OWNER(g:cloud-ps)

DEPENDS(
    infra/dsl_jsonnet/tools/jsonnet
    contrib/python/juggler_sdk/cli
)

DATA(
    arcadia/cloud/ps/monitoring/jsonnet
)

REQUIREMENTS(
    network:full
)

RUN(
    NAME generate_json
    jsonnet ${ARCADIA_ROOT}/cloud/ps/monitoring/jsonnet/init.jsonnet
    STDOUT output.json
)

RUN(
    NAME try_juggler_cli
    STDIN output.json
    juggler_cli load -c
)

END()
