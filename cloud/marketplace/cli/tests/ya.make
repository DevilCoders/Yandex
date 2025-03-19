PY3TEST()

OWNER(g:cloud-marketplace)

TEST_SRCS(
    test_migration.py
    management/__init__.py
    management/commands/__init__.py
    management/commands/test_drop_db.py
    management/commands/test_migrate.py
    management/commands/test_populate_db.py
)

PEERDIR(
    cloud/marketplace/cli/yc_marketplace_cli
    cloud/marketplace/migrations/yc_marketplace_migrations
    cloud/bitbucket/python-common
    contrib/python/pytest-mock
)

END()


