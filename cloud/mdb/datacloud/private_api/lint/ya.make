EXECTEST()

OWNER(g:mdb)

RUN(
    NAME prototool-lint
    sh -c 'prototool lint --debug .'
    ENV PATH=/bin:/usr/bin:${ARCADIA_BUILD_ROOT}/cloud/mdb/datacloud/private_api/lint/prototool:/usr/local/bin
    ENV HOME=${TEST_WORK_ROOT}
    CWD ${ARCADIA_ROOT}/cloud/mdb/datacloud/private_api
)

DATA(arcadia/cloud/mdb/datacloud/private_api)
DATA(arcadia/contrib/libs/googleapis-common-protos)

DEPENDS(cloud/mdb/datacloud/private_api/lint/prototool)

REQUIREMENTS(
    network:full
    dns:dns64
)

END()

RECURSE(prototool)
