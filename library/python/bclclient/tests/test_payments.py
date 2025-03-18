from datetime import datetime
from uuid import uuid4


def test_probe(bcl_test, set_service_alias, acc_psys, response_mocked):

    set_service_alias(client=bcl_test, alias='market')

    Payment = bcl_test.payments.cls_payment
    pay1_id = f'{uuid4()}'
    pay2_id = f'{uuid4()}'

    payments = [
        Payment(
            id=pay1_id,
            amount='12',
            acc_from='111222333',
            acc_to='123456',
            purpose='from API for success probe',
        ),
        Payment(
            id=pay2_id,
            amount='13',
            acc_from=acc_psys,
            acc_to='123456',
            purpose='from API for failed probe',
        ),
    ]

    out = '{"meta": {"built": "2020-03-23T07:54:04.020"}, "data": {"items": [{"status_bcl": 2, "status_remote": 10005, "status_remote_hint": "Payee was not found. Audit ID: 83067994", "id": "' + pay1_id + '"}]}, "errors": [{"id": "' + pay2_id + '", "msg": "Unhandled exception", "event_id": "790e9f0c540d4d76b05e9d7e0a6f18bf", "description": "403 Client Error:  for url: https://calypso.yamoney.ru:9094/webservice/depositio"}]}'

    with response_mocked(f'POST https://balalayka-test.paysys.yandex-team.ru/api/payments/probation/ -> 200:{out}'):
        succeed, failed = bcl_test.payments.probe(*payments)

    assert len(succeed) == 1
    assert len(failed) == 1

    assert payments[0] in succeed
    assert payments[1] in failed
    assert 'Unhandled exception Event ID ' in payments[1].error


def test_register_and_status(bcl_test, acc_bank, acc_psys, response_mocked):

    Payment = bcl_test.payments.cls_payment

    pay1_id = f'{uuid4()}'
    pay2_id = f'{uuid4()}'
    pay3_id = f'{uuid4()}'
    pay4_id = f'{uuid4()}'
    pay5_id = f'{uuid4()}'
    pay6_id = f'{uuid4()}'

    payments = [
        Payment(
            id=pay1_id,
            amount='10.5',
            acc_from=acc_bank,
            acc_to=acc_bank,
            purpose='from API for success',
        ),
        Payment(
            id=pay2_id,
            amount='5.2',
            acc_from=acc_psys,
            acc_to='123456',
            purpose='from API for success',
        ),
        Payment(
            id=pay3_id,
            amount='6',
            acc_from=acc_bank,
            acc_to=acc_bank,
            purpose='from API for cancel success',
        ),
        Payment(
            id=pay4_id,
            amount='7',
            acc_from=acc_psys,
            acc_to='123456',
            purpose='from API for cancel and revoke failed',
        ),
        Payment(
            id=pay5_id,
            amount='8',
            acc_from=acc_bank,
            acc_to=acc_bank,
            purpose='from API for revoke success',
        ),
        Payment(
            id=pay6_id,
            amount='3.0',
            acc_from='nonexistent',
            acc_to='nonexistent',
            purpose='from API for error',
        ),
    ]

    out = '{"meta": {"built": "2020-03-23T07:55:33.636"}, "data": {"items": [{"id": "' + pay1_id + '", "number": "5709", "updated": false}, {"id": "' + pay2_id + '", "number": "3390519", "updated": false}, {"id": "' + pay3_id + '", "number": "5710", "updated": false}, {"id": "' + pay4_id + '", "number": "3390520", "updated": false}, {"id": "' + pay5_id + '", "number": "5711", "updated": false}]}, "errors": [{"id": "' + pay6_id + '", "msg": "Account \\"nonexistent\\" is not registered."}]}'

    # Регистрируем.
    with response_mocked(f'POST https://balalayka-test.paysys.yandex-team.ru/api/payments/ -> 200:{out}'):
        succeed, failed = bcl_test.payments.register(*payments)

    assert len(succeed) == 5
    assert succeed[0].number
    assert not succeed[0].error

    assert len(failed) == 1
    assert not failed[0].number
    assert failed[0].error == 'Account "nonexistent" is not registered.'

    out = '{"meta": {"built": "2020-03-23T07:56:06.358"}, "data": {"items": [{"id": "' + pay3_id + '"}]}, "errors": [{"id": "' + pay4_id + '", "msg": "Restricted."}]}'
    # Аннулируем.
    with response_mocked(f'POST https://balalayka-test.paysys.yandex-team.ru/api/payments/cancellation/ -> 200:{out}'):
        succeed, failed = bcl_test.payments.cancel(ids=[
            payments[2], payments[3],
        ])
    assert len(succeed) == 1
    assert len(failed) == 1
    assert payments[2] in succeed
    assert payments[3] in failed
    assert payments[3].error == 'Restricted.'

    out = '{"meta": {"built": "2020-03-23T08:23:43.466"}, "data": {}, "errors": [{"id": "' + pay4_id + '", "msg": "Restricted."}, {"id": "' + pay5_id + '", "msg": "Restricted."}]}'
    # Отзываем.
    with response_mocked(f'POST https://balalayka-test.paysys.yandex-team.ru/api/payments/revocation/ -> 200:{out}'):
        succeed, failed = bcl_test.payments.revoke(ids=[
            payments[3], payments[4],
        ])

    assert len(succeed) == 0
    assert len(failed) == 2
    assert payments[3] in failed
    assert payments[3].error == 'Restricted.'
    assert payments[4] in failed

    out = ('{"meta": {"built": "2020-03-23T08:34:04.464"}, "data": {"items": ['
            '{"id": "' + pay2_id + '", "number": 3390533, "associate": "yad", "service": "qa", "amount": "5.20", "currency": "RUB", "purpose": "from API for success", "f_acc": "200765", "t_acc": "123456", "status": "processing", "processing_notes": "", "remote_responses": [], "params": {}, "metadata": {}}, '
            '{"id": "' + pay1_id + '", "number": 5730, "associate": "sber", "service": "qa", "amount": "10.50", "currency": "RUB", "purpose": "from API for success", "f_acc": "40702810538000111471", "t_acc": "40702810538000111471", "status": "new", "processing_notes": "", "remote_responses": [], "params": {}, "metadata": {}}, '
            '{"id": "' + pay3_id + '", "number": 5731, "associate": "sber", "service": "qa", "amount": "6.00", "currency": "RUB", "purpose": "from API for cancel success", "f_acc": "40702810538000111471", "t_acc": "40702810538000111471", "status": "cancelled", "processing_notes": "", "remote_responses": [], "params": {}, "metadata": {}}, '
            '{"id": "' + pay4_id + '", "number": 3390534, "associate": "yad", "service": "qa", "amount": "7.00", "currency": "RUB", "purpose": "from API for cancel and revoke failed", "f_acc": "200765", "t_acc": "123456", "status": "processing", "processing_notes": "", "remote_responses": [], "params": {}, "metadata": {}}, '
            '{"id": "' + pay5_id + '", "number": 5732, "associate": "sber", "service": "qa", "amount": "8.00", "currency": "RUB", "purpose": "from API for revoke success", "f_acc": "40702810538000111471", "t_acc": "40702810538000111471", "status": "new", "processing_notes": "", "remote_responses": [], "params": {}, "metadata": {}}]}, "errors": []}')
    # Получаем статусы.

    with response_mocked(f'GET https://balalayka-test.paysys.yandex-team.ru/api/payments/-> 200:{out}'):
        result = bcl_test.payments.get(
            ids=payments + [str(payments[1])],
            upd_since=datetime.now(),
            upd_till='2030-02-10',
        )

    assert len(result) == 5
    assert payments[0].props['status'] == 'new'
    assert payments[1].props['status'] == 'processing'
    assert payments[2].props['status'] == 'cancelled'
    assert payments[3].props['status'] == 'processing'
    assert payments[4].props['status'] == 'new'
