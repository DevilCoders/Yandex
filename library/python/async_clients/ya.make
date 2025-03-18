PY3_LIBRARY()

OWNER(
    g:tools-python,
    smosker
)

PEERDIR(
    contrib/python/aiohttp
    contrib/python/tenacity
)


PY_SRCS(
    TOP_LEVEL
    async_clients/clients/base.py
    async_clients/clients/webmaster.py
    async_clients/clients/fouras.py
    async_clients/clients/gendarme.py
    async_clients/clients/regru.py
    async_clients/clients/sender.py
    async_clients/clients/passport.py
    async_clients/clients/connect.py
    async_clients/clients/mailsettings.py
    async_clients/exceptions/base.py
    async_clients/exceptions/webmaster.py
    async_clients/exceptions/fouras.py
    async_clients/exceptions/gendarme.py
    async_clients/exceptions/passport.py
    async_clients/auth_types.py
    async_clients/utils.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
