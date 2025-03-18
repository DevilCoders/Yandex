PY3_LIBRARY()

OWNER(g:ci)

PY_SRCS(
    __init__.py
)

TASKLET_REG(InfraClosePy py ci.tasklet.registry.common.monitoring.infra_close.impl:InfraCloseImpl)

PEERDIR(
    ci/tasklet/registry/common/monitoring/infra_close/proto

    tasklet/services/ci
    tasklet/services/yav
)

END()
