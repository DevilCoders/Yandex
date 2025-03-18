PY3_LIBRARY()

OWNER(g:tasklet)

PY_SRCS(
    __init__.py
)

TASKLET_REG(WoodcutterPy py ci.tasklet.registry.demo.woodflow.woodcutter.py_impl:WoodcutterImpl)

PEERDIR(
    ci/tasklet/registry/demo/woodflow/woodcutter/proto

    tasklet/services/ci
)

END()
