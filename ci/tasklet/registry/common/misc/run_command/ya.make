PY3_PROGRAM()

OWNER(g:ci)

PY_MAIN(tasklet.cli.main)

PEERDIR(
    ci/tasklet/registry/common/misc/run_command/impl

    tasklet/cli
)

END()

RECURSE(
    impl
    proto
)
