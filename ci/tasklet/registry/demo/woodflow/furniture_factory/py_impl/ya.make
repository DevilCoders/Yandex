PY3_LIBRARY()

OWNER(g:tasklet)

PY_SRCS(
    __init__.py
)

TASKLET_REG(FurnitureFactoryPy py ci.tasklet.registry.demo.woodflow.furniture_factory.py_impl:FurnitureFactoryImpl)

PEERDIR(
    ci/tasklet/registry/demo/woodflow/furniture_factory/proto

    tasklet/services/ci
)

END()
