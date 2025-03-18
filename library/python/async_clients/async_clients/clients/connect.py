from typing import Optional

from async_clients.auth_types import TVM2
from async_clients.clients.base import BaseClient


class Client(BaseClient):
    AUTH_TYPES = {TVM2, }

    async def notify_about_domain_occupied(
        self,
        domain: str,
        new_owner_org_id: int,
        new_owner_new_domain_is_master: bool,
        old_owner_org_id: Optional[int],
        old_owner_new_master_name: Optional[str],
        old_owner_new_master_tech: Optional[bool],
        registrar_id: Optional[int],
    ):
        json = {
            'domain': domain,
            'new_owner': {
                'org_id': new_owner_org_id,
                'new_domain_is_master': new_owner_new_domain_is_master,
            },
        }
        if old_owner_org_id:
            json['old_owner'] = {
                'org_id': old_owner_org_id,
                'new_master': old_owner_new_master_name,
                'tech': old_owner_new_master_tech,
            }
        if registrar_id:
            json['registrar_id'] = registrar_id

        return await self._make_request(
            method='post',
            path='/domenator/event/domain-occupied/',
            json=json,
        )

    async def get_domains(self, org_id: str):
        headers = {'X-ORG-ID': org_id}
        return await self._make_request(
            method='get',
            path='/v3/domains/',
            headers=headers,
        )
