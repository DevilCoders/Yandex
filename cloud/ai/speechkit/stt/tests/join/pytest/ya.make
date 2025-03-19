OWNER(
    myutman
)

PY2TEST()

TEST_SRCS(
    test.py
    testlib.py
)

DEPENDS(
    cloud/ai/speechkit/stt/bin/join
)

DATA(
    arcadia/cloud/ai/speechkit/stt/tests/join/pytest/input_data
)

SIZE(
    MEDIUM
)

TIMEOUT(
    600
)

END()