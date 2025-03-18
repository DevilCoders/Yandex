import pytest

from async_clients.utils import get_client
from async_clients.exceptions.fouras import FourasException

pytestmark = pytest.mark.asyncio


@pytest.fixture
def client():
    return get_client(
        client='fouras',
        host='https://test.fouras.mail.yandex.net',
        service_ticket='some_ticket',
    )


async def test_fouras_domain_status(client, test_vcr):
    with test_vcr.use_cassette('test_fouras_domain_status.yaml'):
        response = await client.domain_status(
            domain='music.yandex.ru',
        )
    assert response == {
        'changed': True,
        'domain': 'music.yandex.ru',
        'enabled': False,
        'public_key': 'key',
        'selector': 'mail',
    }


async def test_fouras_domain_status_error(client, test_vcr):
    with pytest.raises(FourasException):
        with test_vcr.use_cassette('test_fouras_domain_status_error.yaml'):
            await client.domain_status(
                domain='music.yandex.ru',
            )


async def test_fouras_gen_key(client, test_vcr):
    with test_vcr.use_cassette('test_fouras_gen_key.yaml'):
        response = await client.domain_key_gen(
            domain='music.yandex.ru',
        )

    assert response == {
        'changed': True,
        'domain': 'music.yandex.ru',
        'enabled': False,
        'private_key': 'private_key',
        'public_key': 'public_key',
        'selector': 'mail',
    }


async def test_fouras_get_or_gen_domain_key(client, test_vcr):
    with test_vcr.use_cassette('test_fouras_get_or_gen_domain_key.yaml'):
        response = await client.get_or_gen_domain_key(
            domain='music.yandex.ru',
        )

    assert response == 'public_key_gen'
