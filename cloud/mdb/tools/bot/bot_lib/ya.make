PY3_LIBRARY(bot_lib)

STYLE_PYTHON()

OWNER(g:mdb)

PY_SRCS(
    NAMESPACE bot_lib
    __init__.py
    bot_client.py
    enrich.py
    fields.py
)

END()
