PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PY_SRCS(run_teamcity_build.py)

TASKLET_REG(
    RunTeamcityBuild
    py
    cloud.mdb.sandbox.tasklets.teamcity.impl.run_teamcity_build:RunBuildImpl
)

PEERDIR(
    cloud/mdb/sandbox/tasklets/teamcity/proto
    logbroker/mops/testing
    tasklet/services/ci
    tasklet/services/yav
)

END()
