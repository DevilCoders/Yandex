OWNER(g:cloud-nbs)

EXECTEST()

DEPENDS(
    vendor/cuelang.org/go/cmd/cue
)

DATA(
    arcadia/cloud/storage/iac/spinnaker
)

RUN(
    NAME cue_vet
    /bin/sh -c 'cue vet -c -d "#Pipeline" pipelines/*/*/*.json spinnaker-pipeline.schema.cue'

    CWD ${ARCADIA_ROOT}/cloud/storage/iac/spinnaker
)

END()

RECURSE(
    pipelines
    pipeline_converter
)
