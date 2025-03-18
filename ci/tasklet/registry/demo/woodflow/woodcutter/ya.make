PY3_PROGRAM()

OWNER(g:tasklet)

PY_MAIN(tasklet.cli.main)

PEERDIR(
    ci/tasklet/registry/demo/woodflow/woodcutter/proto
    ci/tasklet/registry/demo/woodflow/woodcutter/py_impl

    tasklet/cli
)

END()

RECURSE_FOR_TESTS(tests)

