PY3TEST()

TEST_SRCS(
    conftest.py
    test_ok_client.py
)

PEERDIR(
    contrib/python/pytest
    contrib/python/requests
    contrib/python/mock
    contrib/python/requests-mock
    contrib/python/typeguard
    library/python/ok_client
)

RESOURCE(
    library/python/ok_client/tests/resources/ok_approvement/7b884aa8-4a2f-45da-a719-acbda470c833.json ok_approvement/7b884aa8-4a2f-45da-a719-acbda470c833.json
    library/python/ok_client/tests/resources/ok_approvement/98d51c0a-6831-4d15-a069-ebe81ce3dc05.json ok_approvement/98d51c0a-6831-4d15-a069-ebe81ce3dc05.json
    library/python/ok_client/tests/resources/ok_approvement/d7bd8796-4835-4fb8-a9ab-20a7232e99f3.json ok_approvement/d7bd8796-4835-4fb8-a9ab-20a7232e99f3.json
)

END()
