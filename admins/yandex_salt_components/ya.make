PY3_PROGRAM(yandex_salt_components)
OWNER(g:nocdev-ops)

PEERDIR(
    # salt deps

    # contrib/python/CherryPy # need updates
    contrib/python/dateutil
    contrib/python/distro
    contrib/python/docker
    contrib/python/Jinja2
    contrib/python/M2Crypto
    contrib/python/Mako
    contrib/python/msgpack
    contrib/python/mysqlclient
    contrib/python/kubernetes
    contrib/python/psutil
    contrib/python/pylxd
    contrib/python/python-gnupg
    contrib/python/PyYAML
    contrib/python/pyzmq
    contrib/python/requests
    contrib/python/setproctitle
    contrib/python/six
    contrib/python/smmap
    contrib/python/timelib
    contrib/python/tornado/tornado-4

    # yandex components deps
    library/python/vault_client

    sandbox/common/rest
    sandbox/common/types

    # dowser (https://github.com/torkve/dowser) depends NOCDEV-7488
    contrib/python/aiohttp
    contrib/python/Pillow
)

PY_SRCS(
    MAIN salt-main.py
    monkey_patching.py
    fileserver/dynamic_roots.py
    grains/cauth.py
    grains/conductor.py
    grains/invapi.py
    grains/yandex_common.py
    modules/conductor.py
    modules/invapi.py
    modules/ldmerge.py
    modules/yav.py
    pillar/dynamic_roots.py
    states/monrun.py
    states/yafile.py
    states/yanetconfig.py
    misc/arc2salt/__init__.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
