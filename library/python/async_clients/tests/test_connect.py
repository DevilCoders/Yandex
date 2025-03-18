import pytest

from async_clients.clients.connect import Client as ConnectClient
from async_clients.utils import get_client

pytestmark = pytest.mark.asyncio


@pytest.fixture
def client():
    return get_client(
        client='connect',
        host='https://api-internal-test.directory.ws.yandex.net',
        service_ticket='some_ticket',
    )


async def test_notify_about_domain_occupied(client: ConnectClient, test_vcr):
    with test_vcr.use_cassette('test_notify_about_domain_occupied.yaml'):
        await client.notify_about_domain_occupied(
            domain='domain.com',
            new_owner_org_id=2,
            new_owner_new_domain_is_master=True,
            old_owner_org_id=None,
            old_owner_new_master_name=None,
            old_owner_new_master_tech=None,
            registrar_id=None,
        )
