PY3TEST()

OWNER(g:cloud-marketplace)

TEST_SRCS(test_migration.py)

PEERDIR(
    cloud/marketplace/migrations/yc_marketplace_migrations
    cloud/bitbucket/python-common
    contrib/python/pytest-mock
)

END()


