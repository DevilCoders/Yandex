PY3_PROGRAM()

OWNER(g:ci)

PY_MAIN(tasklet.cli.main)

PEERDIR(
    ci/tasklet/registry/demo/woodflow/sawmill/py_impl
    ci/tasklet/registry/demo/woodflow/woodcutter/py_impl
    ci/tasklet/registry/demo/woodflow/furniture_factory/py_impl

    tasklet/cli
)

END()

RECURSE_FOR_TESTS(
    sawmill
    woodcutter
    furniture_factory
)

