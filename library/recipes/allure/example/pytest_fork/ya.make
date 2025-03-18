OWNER(g:yatool)

PY2TEST()

TEST_SRCS(
    test.py
    test1.py
)

FORK_TESTS()

INCLUDE(${ARCADIA_ROOT}/library/recipes/allure/recipe.inc)

PEERDIR(library/python/pytest/allure)

END()
