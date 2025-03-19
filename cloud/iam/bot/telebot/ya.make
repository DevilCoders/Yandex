PY3_PROGRAM(iam-bot)

OWNER(g:cloud-iam)

PEERDIR(
    contrib/python/aiohttp
    contrib/python/cachetools
    contrib/python/clickhouse-driver
    contrib/python/pyTelegramBotAPI
    contrib/python/Jinja2
    contrib/python/PyYAML
    library/python/tvm2
    ydb/public/sdk/python
)

PY_MAIN(cloud.iam.bot.telebot.main:main)

PY_SRCS(
    __init__.py
    authenticator.py
    bot.py
    db.py
    main.py
    clients/__init__.py
    clients/base_client.py
    clients/oauth_client.py
    clients/tvm_client.py
    clients/paste.py
    clients/staff.py
    clients/startrek.py
    commands/__init__.py
    commands/access_log.py
    commands/command.py
    commands/chats.py
    commands/start.py
    commands/startrek.py
)

RESOURCE_FILES(
    PREFIX cloud/iam/bot/telebot/templates/

    templates/access_log.html
    templates/access_log_details.paste
    templates/startrek_worklog.html
)

END()
