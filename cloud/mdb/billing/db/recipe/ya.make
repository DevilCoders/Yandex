OWNER(g:mdb)

PY3_PROGRAM(billingdb_recipe)

PEERDIR(cloud/mdb/recipes/postgresql/lib)

PY_SRCS(MAIN recipe.py)

END()

RECURSE_FOR_TESTS(
    tests
)
