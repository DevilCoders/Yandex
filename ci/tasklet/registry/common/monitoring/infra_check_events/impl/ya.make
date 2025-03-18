PY3_LIBRARY()

OWNER(g:ci)

PY_SRCS(
    __init__.py
)

TASKLET_REG(InfraCheckEventsPy py ci.tasklet.registry.common.monitoring.infra_check_events.impl:InfraCheckEventsImpl)

PEERDIR(
    ci/tasklet/registry/common/monitoring/infra_check_events/proto
    ci/tasklet/registry/common/monitoring/infra_create/proto
    tasklet/services/ci
)

END()
