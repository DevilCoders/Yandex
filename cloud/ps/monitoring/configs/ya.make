EXECTEST()

OWNER(g:cloud-ps)

RUN(
    jupycli --dry-run --full -d
)

TAG(
    ya:external
)

REQUIREMENTS(
    network:full
)

DATA(
    arcadia/cloud/ps/monitoring/configs
)

DEPENDS(
    music/tools/jupy
)

TEST_CWD(cloud/ps/monitoring)

END()
