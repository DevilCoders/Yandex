OWNER(g:mdb)

PY3_PROGRAM(cms_recipe)

STYLE_PYTHON()

PEERDIR(
    cloud/mdb/recipes/postgresql/lib
)

PY_SRCS(
    MAIN
    recipe.py
)

END()

RECURSE(
    tests
)
