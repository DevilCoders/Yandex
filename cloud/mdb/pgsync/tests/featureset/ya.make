OWNER(g:mdb)

EXECTEST()

RUN(
    NAME consistency
    bash -c 'diff -u <(find features/ -type f | sort) <(sort pgsync.featureset)'
    ENV PATH=/bin:/usr/bin
    CWD ${ARCADIA_ROOT}/cloud/mdb/pgsync/tests
)
DATA(arcadia/cloud/mdb/pgsync/tests)

END()
