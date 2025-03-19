PY3_PROGRAM()

STYLE_PYTHON()

OWNER(g:mdb)

PY_MAIN(tasklet.cli.main)

PEERDIR(
    cloud/mdb/sandbox/tasklets/teamcity/proto
    cloud/mdb/sandbox/tasklets/teamcity/impl
    tasklet/cli
)

END()
