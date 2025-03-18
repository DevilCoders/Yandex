
def test_ping(bcl_test, response_mocked):

    out = '{"meta": {"built": "2020-03-23T07:49:40.516"}, "data": {}, "errors": []}'
    with response_mocked(f'GET https://balalayka-test.paysys.yandex-team.ru/api/ping/ -> 200:{out}'):
        assert bcl_test.utils.ping()
