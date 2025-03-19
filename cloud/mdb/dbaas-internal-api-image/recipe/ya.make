OWNER(g:mdb)

PY3_PROGRAM(dbaas_internal_api_recipe)

STYLE_PYTHON()

PEERDIR(
    cloud/mdb/dbaas-internal-api-image/recipe/lib
    library/python/testing/recipe
)

PY_SRCS(
    MAIN
    recipe.py
)

END()

RECURSE(
    helpers
    lib
    tests
)
