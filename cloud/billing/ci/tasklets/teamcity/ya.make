PY3_PROGRAM()

OWNER(g:cloud-billing)

PY_MAIN(tasklet.cli.main)

PEERDIR(
    cloud/billing/ci/tasklets/teamcity/proto
    cloud/billing/ci/tasklets/teamcity/impl

    tasklet/cli
)

END()
