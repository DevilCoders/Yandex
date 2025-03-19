OWNER(
    g:music
)

PY3_PROGRAM(yasm_dashboard)

PY_SRCS(
    MAIN main.py
    src/__init__.py
    src/log/__init__.py
    src/parse_config.py
    src/yasm/__init__.py
    src/yasm/Yasm.py
    src/yasm/YasmCheck.py
    src/yasm/YasmDashboard.py
    src/Platform/__init__.py
    src/Platform/Exceptions.py
    src/Platform/Platform.py
    src/Platform/Schemas.py
)

PEERDIR(
    contrib/python/requests
    contrib/python/PyYAML
    contrib/python/retry
    contrib/python/marshmallow-dataclass
    admins/yaconfig
)

RESOURCE(
    logging.yaml /logging
)


END()
