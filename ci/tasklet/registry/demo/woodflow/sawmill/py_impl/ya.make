PY3_LIBRARY()

OWNER(g:tasklet)

PY_SRCS(
    __init__.py
)

TASKLET_REG(SawmillPy py ci.tasklet.registry.demo.woodflow.sawmill.py_impl:SawmillImpl)

PEERDIR(
    ci/tasklet/registry/demo/woodflow/sawmill/proto

    tasklet/services/ci
)

END()
