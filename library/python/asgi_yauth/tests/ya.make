PY3TEST()

OWNER(
    smosker
    g:tools-python
)

PEERDIR(
    library/python/asgi_yauth
    contrib/python/pytest-asyncio
    contrib/python/vcrpy
    contrib/python/mock
    contrib/python/requests
    contrib/python/fastapi
)

TEST_SRCS(
    conftest.py
    test_config.py
    test_tvm2.py
)

DATA(
    arcadia/library/python/asgi_yauth/vcr_cassettes
)

END()
