PY23_TEST()

OWNER(g:passport_infra)

PEERDIR(
    library/python/tvmauth
)

TEST_SRCS(
    test_roles.py
)

# tirole
INCLUDE(${ARCADIA_ROOT}/library/recipes/tirole/recipe.inc)
USE_RECIPE(
    library/recipes/tirole/tirole
    --roles-dir library/python/tvmauth/src/ut_without_sanitizer/roles
)

# tvmapi - to provide service ticket for tirole
INCLUDE(${ARCADIA_ROOT}/library/recipes/tvmapi/recipe.inc)

# tvmtool
INCLUDE(${ARCADIA_ROOT}/library/recipes/tvmtool/recipe.inc)
USE_RECIPE(
    library/recipes/tvmtool/tvmtool
    library/python/tvmauth/src/ut_without_sanitizer/tvmtool.cfg
    --with-roles-dir library/python/tvmauth/src/ut_without_sanitizer/roles
)

END()
