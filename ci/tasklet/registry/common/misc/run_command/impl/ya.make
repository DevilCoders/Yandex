PY3_LIBRARY()

OWNER(g:ci)

PY_SRCS(
    __init__.py
)

TASKLET_REG(RunCommand py ci.tasklet.registry.common.misc.run_command.impl:RunCommandImpl)

PEERDIR(
    ci/tasklet/registry/common/misc/run_command/proto
    sandbox/common/mds
    sandbox/sdk2
    sandbox/projects/common/vcs
    tasklet/services/yav
    tasklet/services/ci
)

END()
