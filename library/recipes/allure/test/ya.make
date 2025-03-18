PY2TEST()

OWNER(g:yatool dmitko)

TEST_SRCS(
    test_allure_recipe.py
)

PEERDIR(
    devtools/ya/test/tests/lib
)

DATA(
    arcadia/library/recipes/allure
)

SIZE(MEDIUM)

TAG(
    ya:dirty
    ya:external
)

REQUIREMENTS(
    network:full
    cpu:4
    ram:16
)

END()
