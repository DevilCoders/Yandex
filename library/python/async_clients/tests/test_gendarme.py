import pytest

from async_clients.exceptions.gendarme import DomainNotFoundException
from async_clients.utils import get_client

pytestmark = pytest.mark.asyncio


@pytest.fixture
def client():
    return get_client(
        client='gendarme',
        host='https://test.gendarme.mail.yandex.net',
        service_ticket='some-ticket',
    )


async def test_get_status_success(client, test_vcr):
    with test_vcr.use_cassette('test_gendarme_get_status_success.yaml'):
        expected_response = {'spf': {'value': '', 'match': False},
                             'last_check': '2020-08-28T14:48:06.745230+00:00',
                             'mx': {'value': '', 'match': False},
                             'last_added': '2020-07-06T14:12:23.198350+00:00',
                             'dkim': [{'value': '', 'selector': 'mail', 'match': False}],
                             'ns': {'value': 'ns1.expired.reg.ru', 'match': False}}
        response = await client.status('some-domain.com')
        assert response == expected_response


async def test_get_status_404(client, test_vcr):
    with pytest.raises(DomainNotFoundException):
        with test_vcr.use_cassette('test_gendarme_get_status_404.yaml'):
            await client.status('some-domain.com')


async def test_recheck_success(client, test_vcr):
    with test_vcr.use_cassette('test_gendarme_recheck_success.yaml'):
        expected_response = {'status': 'ok', 'response': {}}
        response = await client.recheck('some-domain.com')
        assert response == expected_response
