PY3TEST()

OWNER(g:tasklet)

SET(TASKLET_ROOT ci/tasklet/registry/demo/picklock)
TEST_SRCS(
    test.py
)

PEERDIR(
    ci/tasklet/registry/demo/py/test
)
DEPENDS(
    ${TASKLET_ROOT}
)
DATA(
    arcadia/${TASKLET_ROOT}/input.example.json
)

END()
