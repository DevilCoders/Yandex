PY3TEST()

OWNER(
    smosker
    g:tools-python
)

PEERDIR(
    library/python/async_clients

    contrib/python/pytest-asyncio
    contrib/python/vcrpy
)

TEST_SRCS(
    conftest.py
    test_auth_types.py
    test_base.py
    test_fouras.py
    test_gendarme.py
    test_webmaster.py
    test_passport.py
    test_connect.py
)

DATA(
    arcadia/library/python/async_clients/vcr_cassettes
)

END()
