PY3_LIBRARY()

OWNER(g:cloud-billing)

PY_SRCS(
    run_teamcity_build.py
)

TASKLET_REG(RunBuild py cloud.billing.ci.tasklets.teamcity.impl.run_teamcity_build:RunBuildImpl)

PEERDIR(
    cloud/billing/ci/tasklets/teamcity/proto
    tasklet/services/ci
    tasklet/services/yav
)

END()
