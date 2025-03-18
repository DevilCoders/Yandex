PY3_PROGRAM()

OWNER(g:tasklet)

PY_MAIN(tasklet.cli.main)

PEERDIR(
    ci/tasklet/registry/demo/woodflow/sawmill/proto
    ci/tasklet/registry/demo/woodflow/sawmill/py_impl

    tasklet/cli
)

END()

RECURSE_FOR_TESTS(tests)
