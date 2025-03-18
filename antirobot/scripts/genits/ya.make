PY3_PROGRAM()

OWNER(g:antirobot)

PY_SRCS(
    MAIN main.py
)

PEERDIR(
    contrib/python/click
    contrib/python/PyYAML
    library/python/resource
)

RESOURCE(
    antirobot/config/service_config.json service_config.json
)

END()
