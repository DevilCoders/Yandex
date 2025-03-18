from decimal import Decimal


def test_allday(bcl_test, response_mocked):

    out = (
        '{"meta": {"built": "2020-03-23T08:05:04.628"}, "data": {"items": ['
        '{"account": "TP5CKP49PXXTS", "date": "2020-03-17", "turnover_ct": "76.86", "turnover_dt": "47.05", '
        '"payments": ['
        '{"number": "9c65f7eba76a456caea8974b3fe34f05", "date": "2020-03-17", "amount": "-3.84", "currency": "RUB", "purpose": "PAYPAL_RECEIPT \u043f\u043e \u0434\u043e\u0433\u043e\u0432\u043e\u0440\u0443 00056384.0 \u043e\u0442 18.04.2016", "direction": "IN", "recipient": {"bik": "", "inn": "7750005796", "name": "", "account": "TP5CKP49PXXTS"}}, '
        '{"number": "b11f51fc3e2f4954aa7418139fe8eb3f", "date": "2020-03-17", "amount": "19.37", "currency": "RUB", "purpose": "PAYPAL_TRANSFER \u043f\u043e \u0434\u043e\u0433\u043e\u0432\u043e\u0440\u0443 00056384.0 \u043e\u0442 18.04.2016", "direction": "OUT", "recipient": {"bik": "", "inn": "7750005796", "name": "", "account": "TP5CKP49PXXTS"}}, '
        '{"number": "39168ca821784ada9878a8286d395da2", "date": "2020-03-17", "amount": "80.59", "currency": "RUB", "purpose": "PAYPAL_HOLD \u043f\u043e \u0434\u043e\u0433\u043e\u0432\u043e\u0440\u0443 00056384.0 \u043e\u0442 18.04.2016", "direction": "IN", "recipient": {"bik": "", "inn": "7750005796", "name": "", "account": "TP5CKP49PXXTS"}}, '
        '{"number": "12235adb9fb04caf9135fa15546aa5c6", "date": "2020-03-17", "amount": "19.36", "currency": "RUB", "purpose": "PAYPAL_COMMISSION \u043f\u043e \u0434\u043e\u0433\u043e\u0432\u043e\u0440\u0443 00056384.0 \u043e\u0442 18.04.2016", "direction": "OUT", "recipient": {"bik": "", "inn": "7750005796", "name": "", "account": "TP5CKP49PXXTS"}}, '
        '{"number": "fc2f6927f443484b8ec4f35437c66c43", "date": "2020-03-17", "amount": "0.11", "currency": "RUB", "purpose": "PAYPAL_RECEIPT \u0422\u0435\u0441\u0442\u0438\u0440\u043e\u0432\u0430\u043d\u0438\u0435 BCL \u043f\u043e \u0434\u043e\u0433\u043e\u0432\u043e\u0440\u0443 00056384.0 \u043e\u0442 18.04.2016", "direction": "IN", "recipient": {"bik": "", "inn": "7750005796", "name": "", "account": "TP5CKP49PXXTS"}}, '
        '{"number": "9ece959a85c54427be6394b1045513e2", "date": "2020-03-17", "amount": "0.20", "currency": "RUB", "purpose": "PAYPAL_COMMISSION \u0422\u0435\u0441\u0442\u0438\u0440\u043e\u0432\u0430\u043d\u0438\u0435 BCL \u043f\u043e \u0434\u043e\u0433\u043e\u0432\u043e\u0440\u0443 00056384.0 \u043e\u0442 18.04.2016", "direction": "OUT", "recipient": {"bik": "", "inn": "7750005796", "name": "", "account": "TP5CKP49PXXTS"}}, '
        '{"number": "a61894e831144be4a148510df6a91136", "date": "2020-03-17", "amount": "8.12", "currency": "RUB", "purpose": "PAYPAL_PAYMENT \u0422\u0435\u0441\u0442\u0438\u0440\u043e\u0432\u0430\u043d\u0438\u0435 BCL \u043f\u043e \u0434\u043e\u0433\u043e\u0432\u043e\u0440\u0443 00056384.0 \u043e\u0442 18.04.2016", "direction": "OUT", "recipient": {"bik": "", "inn": "7750005796", "name": "", "account": "TP5CKP49PXXTS"}}], '
        '"balance_opening": "33.82", "balance_closing": "63.63"}]}, "errors": []}')

    with response_mocked(f'GET https://balalayka-test.paysys.yandex-team.ru/api/statements/ -> 200:{out}'):
        result = bcl_test.statements.get(
            accounts=['TP5CKP49PXXTS'],
            on_date='2020-03-17',
        )

    assert len(result) == 1
    statement = result[0]

    assert statement.account == 'TP5CKP49PXXTS'
    assert statement.balance_opening == Decimal('33.82')
    assert statement.balance_closing == Decimal('63.63')
    assert statement.turnover_dt == Decimal('47.05')
    assert statement.turnover_ct == Decimal('76.86')

    assert len(statement.payments) == 7


def test_intraday(bcl_test, response_mocked):

    out = (
        '{"meta": {"built": "2020-03-23T08:05:27.481"}, "data": {"items": ['
        '{"account": "40702810920010001241", "date": "2020-03-18", "turnover_ct": "13.00", "turnover_dt": "0.00", "payments": ['
        '{"number": "111111", "date": "2019-07-01", "amount": "11.00", "currency": "RUB", "purpose": "3d66238f-a135-4062-bc09-d10211cf3331", "direction": "IN", "recipient": {"bik": "666666666", "inn": "118111111", "name": "www www www", "account": "66666666666666666666"}}, '
        '{"number": "111111", "date": "2019-07-01", "amount": "2.00", "currency": "RUB", "purpose": "9b9a5041-690d-4ee3-bc5f-f01489af8a8f", "direction": "IN", "recipient": {"bik": "666666666", "inn": "118111111", "name": "www www www", "account": "66666666666666666666"}}'
        ']}]}, "errors": []}')

    with response_mocked(f'GET https://balalayka-test.paysys.yandex-team.ru/api/statements/ -> 200:{out}'):
        result = bcl_test.statements.get(
            accounts=['40702810920010001241'],
            on_date='2020-03-18',
            intraday=True,
        )

    assert len(result) == 1
    statement = result[0]

    assert str(statement) == '40702810920010001241 [2020-03-18]'

    assert statement.account == '40702810920010001241'
    assert statement.balance_opening is None
    assert statement.balance_closing is None
    assert statement.turnover_dt == 0
    assert statement.turnover_ct == Decimal('13.00')

    assert len(statement.payments) == 2
    assert statement.payments[0].number == '111111'
    assert str(statement.payments[0]) == '111111 [2019-07-01] 11.00 RUB'
