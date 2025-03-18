PY2_LIBRARY()

OWNER(g:antiadblock)

PY_SRCS(
    __init__.py
    config_stub.py
    constants.py
    containers_context.py
    stub_server.py
    util.py
    initial_config.py
    an_yandex_utils.py
)

PEERDIR(
    contrib/python/requests
    contrib/python/pytest
    contrib/python/netifaces
    contrib/python/Werkzeug

    antiadblock/cryprox/cryprox
    antiadblock/libs/decrypt_url
)

END()
