PY3_PROGRAM(metadb_migration_341_helper)

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    contrib/python/psycopg2
)

PY_SRCS(metadb_migration_341_helper.py)

PY_MAIN(cloud.mdb.tools.metadb_migration_341_helper.metadb_migration_341_helper:main)

END()
