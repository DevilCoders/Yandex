PY2_LIBRARY()

OWNER(g:antiadblock solovyev)


PY_SRCS(
    postgres.py
    migrator.py
    exceptions.py
)

PEERDIR(
    contrib/python/psycopg2
    contrib/python/sqlalchemy/sqlalchemy-1.2
)

END()

RECURSE_FOR_TESTS(
    tests
    tests_recipe
)
