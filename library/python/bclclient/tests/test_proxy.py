import pytest
from bclclient.exceptions import ApiCallError


def test_paypal(bcl_test, response_mocked):

    out = '{"meta": {"built": "2020-03-23T07:57:24.395"}, "data": {}, "errors": [{"error_description": "Invalid authorization code", "error": "access_denied", "correlation_id": "210096297f317", "information_link": "https://developer.paypal.com/docs/api/#errors"}]}'
    with pytest.raises(ApiCallError) as e:
        with response_mocked(f'POST https://balalayka-test.paysys.yandex-team.ru/api/proxy/paypal/gettoken/ -> 500:{out}'):
            bcl_test.proxy.paypal.get_token(auth_code='1020')

    assert e.value.msg['error'] == 'access_denied'

    out = '{"meta": {"built": "2020-03-23T07:57:58.644"}, "data": {}, "errors": [{"raw": "Failed. Response status: 401. Response message: Unauthorized. Error message: "}]}'
    with pytest.raises(ApiCallError) as e:
        with response_mocked(f'POST https://balalayka-test.paysys.yandex-team.ru/api/proxy/paypal/getuserinfo/ -> 500:{out}'):
            bcl_test.proxy.paypal.get_userinfo(token='dummytoken')

    assert 'Failed. Response status: 401. Response message: Unauthorized.' in e.value.msg['raw']


def test_payoneer(bcl_test, set_service_alias, response_mocked):

    set_service_alias(client=bcl_test, alias='market')

    out = '{"meta": {"built": "2020-03-23T07:58:55.890"}, "data": {}, "errors": [{"audit_id": 0, "code": 10000, "description": "Unauthorized access", "hint": "please review your credentials and security information"}]}'
    with pytest.raises(ApiCallError) as e:
        with response_mocked(f'POST https://balalayka-test.paysys.yandex-team.ru/api/proxy/payoneer/getloginlink/ -> 500:{out}'):
            bcl_test.proxy.payoneer.get_login_link(program_id='prog1', payee_id='payee1', options={'dummy': 'val'})

    assert e.value.msg['description'] == 'Unauthorized access'

    with pytest.raises(ApiCallError) as e:
        with response_mocked(f'POST https://balalayka-test.paysys.yandex-team.ru/api/proxy/payoneer/getpayeestatus/ -> 500:{out}'):
            bcl_test.proxy.payoneer.get_payee_status(program_id='prog1', payee_id='payee1')

    assert e.value.msg['description'] == 'Unauthorized access'


def test_pingpong(bcl_test, set_service_alias, response_mocked):

    set_service_alias(client=bcl_test, alias='market')

    out = '{"meta": {"built": "2020-03-23T08:00:53.753"}, "data": {}, "errors": [{"msg": "Method Not Allowed: "}]}'

    with pytest.raises(ApiCallError) as e:
        with response_mocked(f'POST https://balalayka-test.paysys.yandex-team.ru/api/proxy/pingpong/getsellerstatus -> 405:{out}'):
            bcl_test.proxy.pingpong.get_seller_status(seller_id='10')

    assert e.value.msg['msg'] == 'Method Not Allowed: '

    with pytest.raises(ApiCallError) as e:
        with response_mocked(f'POST https://balalayka-test.paysys.yandex-team.ru/api/proxy/pingpong/getonboardinglink -> 405:{out}'):
            bcl_test.proxy.pingpong.get_onboarding_link(
                seller_id='10',
                currency='USD',
                country='US',
                store_name='MyStore',
                store_url='http://some.com',
                notify_url='http://some.com',
            )

    assert e.value.msg['msg'] == 'Method Not Allowed: '
