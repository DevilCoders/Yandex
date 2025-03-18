import pytest

from async_clients.utils import get_client
from async_clients.clients.passport import Client
from async_clients.exceptions.passport import PassportException

pytestmark = pytest.mark.asyncio


@pytest.fixture
def client():
    return get_client(
        client='passport',
        host='http://passport-test-internal.yandex.ru/',
        consumer_query_param='test',
    )


async def test_validate_natural_domain_success(client: Client, test_vcr):
    with test_vcr.use_cassette('test_passport_validate_natural_domain_success.yaml'):
        result = await client.validate_natural_domain('domain.com')
        assert result is True


async def test_validate_natural_domain_exception(client: Client, test_vcr):
    with pytest.raises(PassportException) as exc:
        with test_vcr.use_cassette('test_passport_validate_natural_domain_exception.yaml'):
            await client.validate_natural_domain('domain.com')

    assert str(exc.value) == 'domain.invalid'


async def test_set_master_domain_success(client: Client, test_vcr):
    with test_vcr.use_cassette('test_passport_set_master_domain_success.yaml'):
        result = await client.set_master_domain(1, 2)
        assert result is True


async def test_domain_add_success(client: Client, test_vcr):
    with test_vcr.use_cassette('test_passport_domain_add_success.yaml'):
        result = await client.domain_add('domain.com', 123)
        assert result is True


async def test_domain_alias_delete_success(client: Client, test_vcr):
    with test_vcr.use_cassette('test_passport_domain_alias_delete_success.yaml'):
        result = await client.domain_alias_delete(1, 2)
        assert result is True


async def test_domain_alias_delete_with_nonexistent_alias(client: Client, test_vcr):
    with test_vcr.use_cassette('test_passport_domain_alias_delete_with_nonexistent_alias.yaml'):
        result = await client.domain_alias_delete(1, 2)
        assert result is True


async def test_domain_edit_success(client: Client, test_vcr):
    with test_vcr.use_cassette('test_passport_domain_edit_success.yaml'):
        result = await client.domain_edit(1, {
            'admin_uid': 123,
        })
        assert result is True


async def test_domain_delete_success(client: Client, test_vcr):
    with test_vcr.use_cassette('test_passport_domain_delete_success.yaml'):
        result = await client.domain_delete(1)
        assert result is True


async def test_domain_delete_with_nonexistent_domain(client: Client, test_vcr):
    with test_vcr.use_cassette('test_passport_domain_delete_with_nonexistent_domain.yaml'):
        result = await client.domain_delete(1)
        assert result is True


async def test_validate_connect_domain_success(client: Client, test_vcr):
    with test_vcr.use_cassette('test_passport_validate_connect_domain_success.yaml'):
        result = await client.validate_connect_domain('domain.com')
        assert result is True
