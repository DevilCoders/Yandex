import pytest

from async_clients.utils import get_client
from async_clients.exceptions.webmaster import WebmasterException

pytestmark = pytest.mark.asyncio


@pytest.fixture
def client():
    return get_client(
        client='webmaster',
        host='https://webmaster-internal-back.unstable.common.yandex.net',
        service_ticket='some_ticket',
    )


async def test_parse_errors_success(client, test_vcr):
    with pytest.raises(WebmasterException) as exc:
        with test_vcr.use_cassette('test_parse_errors_success.yaml'):
            await client.info(domain='test.ru', admin_uid=123)

    assert exc.value.code == 'USER__HOST_NOT_ADDED'


async def test_parse_errors_fail(client, test_vcr):
    with pytest.raises(WebmasterException) as exc:
        with test_vcr.use_cassette('test_parse_errors_fail.yaml'):
            await client.info(
                domain='test.ru',
                admin_uid=123,
                ignore_errors=['USER__HOST_NOT_ADDED'],
            )
    assert exc.value.code == 'USER__HOST_ERROR'


async def test_parse_multiple_errors_fail(client, test_vcr):
    with pytest.raises(WebmasterException) as exc:
        with test_vcr.use_cassette('test_parse_multiple_errors_fail.yaml'):
            await client.info(
                domain='test.ru',
                admin_uid=123,
                ignore_errors=['USER__HOST_NOT_ADDED'],
            )
    assert exc.value.code == 'USER__HOST_ERROR'


async def test_parse_errors_ignore_success(client, test_vcr):
    with test_vcr.use_cassette('test_parse_errors_ignore_success.yaml'):
        response = await client.info(
            domain='test.ru',
            admin_uid=123,
            ignore_errors=['USER__HOST_NOT_ADDED'],
        )
    assert response['data'] == {'verification': 'ok'}


async def test_parse_errors_ignore_all_success(client, test_vcr):
    with test_vcr.use_cassette('test_parse_errors_ignore_all_success.yaml'):
        response = await client.info(
            domain='test.ru',
            admin_uid=123,
            ignore_errors=True,
        )
    assert response['data'] == {'verification': 'ok'}
