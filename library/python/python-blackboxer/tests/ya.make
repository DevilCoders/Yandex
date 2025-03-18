PY23_TEST()

OWNER(
    dshmatkov
)

PEERDIR(
    contrib/python/requests
    contrib/python/pytest
    contrib/python/httpretty
    contrib/python/PySocks

    library/python/python-blackboxer
    library/python/yenv
)

DATA(
    arcadia/library/python/python-blackboxer/tests/fixtures/test_check_ip.txt
    arcadia/library/python/python-blackboxer/tests/fixtures/test_check_ip_wrong_nets.txt
    arcadia/library/python/python-blackboxer/tests/fixtures/test_lcookie.txt
    arcadia/library/python/python-blackboxer/tests/fixtures/test_lcookie_wrong_value.txt
    arcadia/library/python/python-blackboxer/tests/fixtures/test_login.txt
    arcadia/library/python/python-blackboxer/tests/fixtures/test_oauth.txt
    arcadia/library/python/python-blackboxer/tests/fixtures/test_oauth_expired.txt
    arcadia/library/python/python-blackboxer/tests/fixtures/test_sessionid.txt
    arcadia/library/python/python-blackboxer/tests/fixtures/test_userinfo.txt
)

TEST_SRCS(
    conftest.py
    test_blackboxer.py
    test_environment.py
    test_utils.py
)

END()

RECURSE_FOR_TESTS(
    test_async
)
