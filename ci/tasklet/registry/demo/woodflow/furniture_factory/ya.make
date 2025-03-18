PY3_PROGRAM()

OWNER(g:tasklet)

PY_MAIN(tasklet.cli.main)

PEERDIR(
    ci/tasklet/registry/demo/woodflow/furniture_factory/proto
    ci/tasklet/registry/demo/woodflow/furniture_factory/py_impl

    tasklet/cli
)

END()

RECURSE_FOR_TESTS(tests)

