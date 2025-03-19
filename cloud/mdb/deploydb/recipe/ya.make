OWNER(g:mdb)

PY3_PROGRAM(deploydb_recipe)

STYLE_PYTHON()

PEERDIR(cloud/mdb/recipes/postgresql/lib)

PY_SRCS(
    MAIN
    recipe.py
)

END()

RECURSE(
    helpers
    tests
)
