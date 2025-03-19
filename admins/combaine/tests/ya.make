EXECTEST()

OWNER(g:music-sre)

RUN(
    NAME stable-configs combaine-generator -t -d -s stable -o ${TEST_OUT_ROOT}/stable
)
RUN(
    NAME testing-configs combaine-generator -t -d -s testing -o ${TEST_OUT_ROOT}/testing
)

DATA(arcadia/admins/combaine/configs)

TEST_CWD(admins/combaine/configs)

DEPENDS(admins/combaine/configs/generator)

END()
