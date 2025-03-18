PY3_LIBRARY()

OWNER(g:ci)

PY_SRCS(
    __init__.py
)

TASKLET_REG(Changelog py ci.tasklet.registry.demo.changelog.impl:ChangelogImpl)

PEERDIR(
    ci/tasklet/registry/demo/changelog/proto

    tasklet/services/ci
    tasklet/services/yav
)

END()
