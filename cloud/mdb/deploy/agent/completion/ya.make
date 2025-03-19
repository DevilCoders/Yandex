UNION()

OWNER(
    g:mdb
)

RUN_PROGRAM(
    cloud/mdb/deploy/agent/cmd/mdb-deploy
    completion
    bash
    STDOUT mdb-deploy.bash
    OUT mdb-deploy.bash
)

END()
