PY3_PROGRAM()

OWNER(g:ci)

PY_MAIN(tasklet.cli.main)

PEERDIR(
    ci/tasklet/registry/common/monitoring/juggler_watch/proto
    ci/tasklet/registry/common/monitoring/juggler_watch/impl

    tasklet/cli
)


END()
