PY2TEST()

OWNER(
    vasil-sd
)

TEST_SRCS(test.py)

TIMEOUT(300)

SIZE(MEDIUM)

FORK_TEST_FILES()

FORK_SUBTESTS()

DATA(
    arcadia/tools/domschemec/tests/data/
)

DEPENDS(
    tools/domschemec
)

END()
