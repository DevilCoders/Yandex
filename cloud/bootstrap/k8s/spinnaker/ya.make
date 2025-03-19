EXECTEST()

DEPENDS(
    vendor/cuelang.org/go/cmd/cue
)

DATA(
    arcadia/cloud/bootstrap/k8s/spinnaker
)

RUN(
    NAME cue_vet
    /bin/sh -c 'cue vet -c -d "#Project" projects/*.json spinnaker-project.schema.cue'
    CWD ${ARCADIA_ROOT}/cloud/bootstrap/k8s/spinnaker
)

RUN(
    NAME cue_vet
    /bin/sh -c 'cue vet -c -d "#Application" applications/*/application.json spinnaker-application.schema.cue'
    CWD ${ARCADIA_ROOT}/cloud/bootstrap/k8s/spinnaker
)

RUN(
    NAME cue_vet
    /bin/sh -c 'cue vet -c -d "#Pipeline" applications/*/pipelines/*.json spinnaker-pipeline.*'
    CWD ${ARCADIA_ROOT}/cloud/bootstrap/k8s/spinnaker
)

END()
