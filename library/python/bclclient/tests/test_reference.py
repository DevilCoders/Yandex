
def test_associates(bcl_test, response_mocked):

    out = '{"meta": {"built": "2020-03-23T08:01:54.148"}, "data": {"items": [{"alias": "jpmorgan", "title": "J.P. Morgan"}, {"alias": "paypal", "title": "PayPal"}, {"alias": "payoneer", "title": "Payoneer"}]}, "errors": []}'
    with response_mocked(f'GET https://balalayka-test.paysys.yandex-team.ru/api/refs/associates/ -> 200:{out}'):
        result = bcl_test.reference.get_associates()
    assert 'title' in result['jpmorgan']


def test_services(bcl_test, response_mocked):

    out = '{"meta": {"built": "2020-03-23T08:04:01.640"}, "data": {"items": [{"alias": "oebs", "title": "OEBS", "tvm_app": null}]}, "errors": []}'
    with response_mocked(f'GET https://balalayka-test.paysys.yandex-team.ru/api/refs/services/ -> 200:{out}'):
        result = bcl_test.reference.get_services()
    assert 'title' in result['oebs']


def test_statuses(bcl_test, response_mocked):

    out = (
        '{"meta": {"built": "2020-03-23T08:04:23.866"}, "data": {"items": ['
        '{"realm": "payments", "statuses": []}, '
        '{"realm": "bundles", "statuses": []}, '
        '{"realm": "statements", "statuses": [{"alias": "new", "title": "\u041d\u043e\u0432\u044b\u0439"}]}]}, '
        '"errors": []}')

    with response_mocked(f'GET https://balalayka-test.paysys.yandex-team.ru/api/refs/statuses/ -> 200:{out}'):
        result = bcl_test.reference.get_statuses()
    assert set(result.keys()) == {'payments', 'bundles', 'statements'}
    assert 'title' in result['statements'][0]


def test_accounts(bcl_test, acc_bank, acc_psys, response_mocked):

    out = '{"meta": {"built": "2020-03-23T08:04:41.480"}, "data": {"items": [{"number": "200765", "blocked": 1}, {"number": "40702810538000111471", "blocked": 0}]}, "errors": []}'
    with response_mocked(f'GET https://balalayka-test.paysys.yandex-team.ru/api/refs/accounts/ -> 200:{out}'):
        result = bcl_test.reference.get_accounts([acc_psys, acc_bank])

    result = {item['number']: item for item in result}

    assert acc_bank in result
    assert acc_psys in result
    assert len(result) == 2
