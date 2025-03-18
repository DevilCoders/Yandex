PY3_PROGRAM()

OWNER(g:tasklet)

PY_MAIN(tasklet.cli.main)

PEERDIR(
    ci/tasklet/registry/demo/picklock/proto
    ci/tasklet/registry/demo/picklock/impl

    tasklet/cli
)

END()

RECURSE_FOR_TESTS(tests)
