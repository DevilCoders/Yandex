PY3_LIBRARY()

OWNER(g:ci)

PY_SRCS(
    __init__.py
)

TASKLET_REG(InfraCreatePy py ci.tasklet.registry.common.monitoring.infra_create.impl:InfraCreateImpl)

PEERDIR(
    ci/tasklet/registry/common/monitoring/infra_create/proto

    tasklet/services/ci
    tasklet/services/yav
)

END()
