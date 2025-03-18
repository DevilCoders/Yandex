from .base import BaseClient
from async_clients.auth_types import TVM2
import json


class Client(BaseClient):
    AUTH_TYPES = {TVM2, }

    async def send_transactional(self, account_slug: str, campaign_slug: str, *, args={}, to_email=None, to_yandex_puid=None):
        data = {
            'args': json.dumps(args)
        }

        if to_email:
            data['to_email'] = to_email
        elif to_yandex_puid:
            data['to_yandex_puid'] = to_yandex_puid
        else:
            raise Exception('to_email or to_yandex_puid should be provided')

        return await self._make_request(
            path=f'/api/0/{account_slug}/transactional/{campaign_slug}/send',
            data=data,
            method='post'
        )
