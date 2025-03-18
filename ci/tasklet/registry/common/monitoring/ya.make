PY3_PROGRAM()

OWNER(g:ci)

PY_MAIN(tasklet.cli.main)

PEERDIR(
    ci/tasklet/registry/common/monitoring/juggler_watch/impl
    ci/tasklet/registry/common/monitoring/infra_create/impl
    ci/tasklet/registry/common/monitoring/infra_close/impl
    ci/tasklet/registry/common/monitoring/infra_check_events/impl

    tasklet/cli
)

END()
