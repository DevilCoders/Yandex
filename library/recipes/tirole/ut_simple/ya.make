PY3TEST()

OWNER(g:passport_infra)

TEST_SRCS(test.py)

PEERDIR(
    library/python/tvmauth
)

# common usage
INCLUDE(${ARCADIA_ROOT}/library/recipes/tirole/recipe.inc)

USE_RECIPE(
    library/recipes/tirole/tirole
    --roles-dir library/recipes/tirole/ut_simple/roles_dir
)

# tvmapi - to provide service ticket for tirole
INCLUDE(${ARCADIA_ROOT}/library/recipes/tvmapi/recipe.inc)

END()
