PY3_PROGRAM()

OWNER(g:ci)

PY_MAIN(tasklet.cli.main)

PEERDIR(
    ci/tasklet/registry/common/monitoring/infra_close/proto
    ci/tasklet/registry/common/monitoring/infra_close/impl

    tasklet/cli
)


END()
