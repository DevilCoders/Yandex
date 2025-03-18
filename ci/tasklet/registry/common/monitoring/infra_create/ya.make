PY3_PROGRAM()

OWNER(g:ci)

PY_MAIN(tasklet.cli.main)

PEERDIR(
    ci/tasklet/registry/common/monitoring/infra_create/proto
    ci/tasklet/registry/common/monitoring/infra_create/impl

    tasklet/cli
)


END()
