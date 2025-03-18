PY2_PROGRAM()

OWNER(g:antiadblock solovyev)


PY_SRCS(
    MAIN recipe.py
)

PEERDIR(
    library/python/testing/recipe
    library/python/testing/yatest_common
    antiadblock/postgres_local
)

END()
