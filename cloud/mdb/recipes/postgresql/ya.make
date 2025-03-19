OWNER(g:mdb)

PY3_PROGRAM(postgresql_recipe)

STYLE_PYTHON()

PEERDIR(
    cloud/mdb/recipes/postgresql/lib
    library/python/testing/recipe
)

PY_SRCS(
    MAIN
    recipe.py
)

END()

RECURSE(
    lib
)
