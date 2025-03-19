PY3_PROGRAM()

OWNER(g:cloud-support)

PEERDIR(
    contrib/python/python-telegram-bot
    contrib/python/psycopg2
    contrib/python/requests
    contrib/python/PyYAML
    contrib/python/sqlalchemy/sqlalchemy-1.2
)


PY_SRCS(
    TOP_LEVEL
    MAIN bot.py
    __init__.py

    # utils
    app/utils/classes.py
    app/utils/config.py
    app/utils/staff.py

    # database modules
    app/database/cache.py
    app/database/components.py
    app/database/db.py
    app/database/models.py
    app/database/users.py
)

END()

