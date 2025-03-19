OWNER(g:mdb)

EXECTEST()

RUN(
    NAME consistency
    bash -c 'diff -u <(find features/ -type f | sort) <(sort gpsync.featureset)'
    ENV PATH=/bin:/usr/bin
    CWD ${ARCADIA_ROOT}/cloud/mdb/gpsync/tests
)
DATA(arcadia/cloud/mdb/gpsync/tests)

END()
