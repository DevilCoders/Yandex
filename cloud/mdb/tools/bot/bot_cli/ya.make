PY3_PROGRAM(bot_cli)

STYLE_PYTHON()

OWNER(g:mdb)

PY_SRCS(
    MAIN
    bh.py
)

PEERDIR(
    cloud/mdb/tools/bot/bot_lib
    contrib/python/click
    contrib/python/pandas
    contrib/python/requests
    contrib/python/tabulate
)

END()
