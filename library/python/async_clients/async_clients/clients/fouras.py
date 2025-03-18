from aiohttp import ClientResponse
from typing import Optional

from .base import BaseClient
from ..auth_types import TVM2
from ..exceptions.fouras import FourasException
from ..exceptions.base import BadResponseStatus


class Client(BaseClient):
    AUTH_TYPES = {TVM2, }

    async def parse_response(self, response: ClientResponse, **kwargs) -> dict:
        response = await super().parse_response(response)

        if response['status'] != 'ok':
            raise FourasException(response=response.get('response', 'No response'))

        return response['response']

    async def domain_status(self, domain: str, selector: Optional[str] = None) -> dict:
        """
        https://wiki.yandex-team.ru/mail/pdd/fouras/#status

        {
            "enabled" : false,
            "domain" : "music.yandex.ru",
            "changed" : true,
            "selector" : "mail",
            "public_key" : "v=DKIM1; k=rsa; t=s; p=..."
        }
        """

        params = {
            'domain': domain,
            'selector': selector or 'mail'
        }
        return await self._make_request(
            path='/domain/status',
            params=params,
        )

    async def domain_key_gen(self, domain: str, selector: Optional[str] = None) -> dict:
        """
        https://wiki.yandex-team.ru/mail/pdd/fouras/#generacijakljucha

        {
            "public_key" : "v=DKIM1; k=rsa; t=s; p=..."
            "private_key": "....",
            "domain": "yandex.ru",
            "selector": "mail",
            "enabled": true
        }
        """

        data = {
            'domain': domain,
            'selector': selector or 'mail'
        }
        return await self._make_request(
            path='/domain/key/gen',
            method='post',
            json=data,
        )

    async def get_or_gen_domain_key(self, domain: str, selector: Optional[str] = None):
        try:
            response = await self.domain_status(domain=domain)
        except BadResponseStatus as exc:
            if exc.status == 404:
                response = await self.domain_key_gen(domain=domain, selector=selector)
            else:
                raise
        return response['public_key']
