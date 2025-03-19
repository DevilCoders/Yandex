OWNER(g:cloud-marketplace)

PY3_LIBRARY()

PY_SRCS(
    __init__.py
    core.py
    command.py
    types.py
    commands/__init__.py
    commands/drop_db.py
    commands/migrate.py
    commands/populate_db.py
)

PEERDIR(
    cloud/marketplace/common/yc_marketplace_common
    cloud/marketplace/cli/yc_marketplace_cli/migrations
)

END()
