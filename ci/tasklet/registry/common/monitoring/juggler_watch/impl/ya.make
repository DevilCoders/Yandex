PY3_LIBRARY()

OWNER(g:ci)

PY_SRCS(
    __init__.py
)

TASKLET_REG(JugglerWatchPy py ci.tasklet.registry.common.monitoring.juggler_watch.impl:JugglerWatchImpl)

PEERDIR(
    ci/tasklet/registry/common/monitoring/juggler_watch/proto

    tasklet/services/ci
)

END()
