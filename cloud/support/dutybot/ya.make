PY3_PROGRAM()

OWNER(g:cloud-support)

PEERDIR(
    contrib/python/python-telegram-bot
    contrib/python/psycopg2
    contrib/python/boto3
    contrib/python/requests
    contrib/python/PyYAML
    contrib/python/sqlalchemy/sqlalchemy-1.2
    library/python/startrek_python_client
)


PY_SRCS(
    TOP_LEVEL
    MAIN bot.py
    __init__.py

    # app modules 
    # utils
    app/utils/classes.py
    app/utils/config.py
    app/utils/resps.py
    app/utils/staff.py

    # addons
    app/addons/forward_tickets.py
    app/addons/newform_resolver.py
    app/addons/startrek_updater.py

    # database modules
    app/database/cache.py
    app/database/chats.py
    app/database/components.py
    app/database/db.py
    app/database/logs.py
    app/database/models.py
    app/database/users.py
)

END()

