PY3_LIBRARY()

OWNER(g:ci)

PY_SRCS(
    __init__.py
)

TASKLET_REG(Picklock py ci.tasklet.registry.demo.picklock.impl:PicklockImpl)

PEERDIR(
    ci/tasklet/registry/demo/picklock/proto

    tasklet/services/ci
    tasklet/services/yav
)

END()
