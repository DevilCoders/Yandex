OWNER(
    g:music
)

PY3_PROGRAM(platform-balancer-info)

PY_SRCS(
    MAIN main.py
    commands.py
    utils/log.py
    utils/Yasm.py
    utils/Platform/__init__.py
    utils/Platform/Platform.py
    utils/Platform/Exceptions.py
    utils/Platform/Schemas.py
)

PEERDIR(
    contrib/python/requests
    contrib/python/PyYAML
    contrib/python/retry
    contrib/python/marshmallow-dataclass
)

END()
