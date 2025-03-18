from .base import BaseClient
from ..auth_types import TVM2
import json

from typing import Optional, Any

API_PREFIX = 'api/regru2'
CREATE_DOMAIN_API_PATH = f'{API_PREFIX}/domain/create'

DEFAULT_PERIOD = 1

MAX_RETRIES_INCLUDING_FIRST_ATTEMPT = 4


class Client(BaseClient):
    AUTH_TYPES = (TVM2,)
    USE_ZORA = True
    TIMEOUT = 30

    def __init__(self, *args, use_zora, username: str, password: str, contacts: dict, **kwargs):
        super().__init__(*args, **kwargs)
        self.username = username
        self.password = password
        self.contacts = contacts

        self.USE_ZORA = use_zora
        if not use_zora:
            self.AUTH_TYPES = tuple()

    def get_retries(
            self,
            path: str,
            method: str = 'get',
            params: Optional[dict] = None,
            json: Optional[dict] = None,
            data: Optional[Any] = None,
            headers: Optional[dict] = None,
            timeout: int = None,
            **kwargs
    ):

        if path == CREATE_DOMAIN_API_PATH:
            return 1

        return MAX_RETRIES_INCLUDING_FIRST_ATTEMPT

    async def register(self, *, domain_name: str, nss: dict, enduser_ip: str, contacts: dict, period=DEFAULT_PERIOD, reg_premium=0):
        """
        API https://www.reg.ru/support/help/api2#domain_create
        """
        assert period == 1 or period == 2, "period can only be 1 or 2"
        assert period == 0 or period == 1, "reg_premium can only be 0 or 1"

        params = {
            "input_format": "json",
            "username": self.username,
            "password": self.password,
            "period": str(period),
            "enduser_ip": enduser_ip,
            "input_data": json.dumps({
                "io_encoding": "utf8",
                "output_content_type": "application/json",
                "contacts": contacts,
                "nss": nss
            }),
            "output_format": "json",
            "domain_name": domain_name,
            "reg_premium": str(reg_premium)
        }

        return await self._make_request(CREATE_DOMAIN_API_PATH, params=params)

    async def get_info(self, service_ids: [str]):
        params = {
            "input_format": "json",
            "username": self.username,
            "password": self.password,
            "input_data": json.dumps({
                "io_encoding": "utf8",
                "output_content_type": "application/json",
                "services": [{"service_id": x} for x in service_ids],
            }),
            "output_format": "json",
        }

        return await self._make_request(f'{API_PREFIX}/service/get_info', params=params)

    async def get_info_by_domain(self, domain: str):
        params = {
            "input_format": "json",
            "username": self.username,
            "password": self.password,
            "input_data": json.dumps({
                "io_encoding": "utf8",
                "output_content_type": "application/json",
                "services": [{"domain_name": domain}],
            }),
            "output_format": "json",
        }

        return await self._make_request(f'{API_PREFIX}/service/get_info', params=params)

    async def renew(self, service_id, period=DEFAULT_PERIOD):
        params = {
            "username": self.username,
            "password": self.password,
            "period": period,
            "service_id": service_id,
            "output_content_type": "application/json",
            "output_format": "json",
        }

        return await self._make_request('api/regru2/service/renew', params=params)

    async def zone_add_mx(self, domains, subdomain='@', priority=0, mail_server='mx.yandex.ru'):

        if type(domains) == str:
            domains = [domains]

        params = {
            "input_format": "json",
            "username": self.username,
            "password": self.password,
            "input_data": json.dumps({
                "domains": [{"dname": domain} for domain in domains],
                "subdomain": subdomain,
                "mail_server": mail_server,
                "output_content_type": "application/json",
            }),
            "output_format": "json",
        }

        return await self._make_request(f'{API_PREFIX}/zone/add_mx', params=params, timeout=120)

    async def zone_add_txt(self, domains, subdomain='@', text=''):

        if type(domains) == str:
            domains = [domains]

        params = {
            "input_format": "json",
            "username": self.username,
            "password": self.password,
            "input_data": json.dumps({
                "domains": [{"dname": domain} for domain in domains],
                "subdomain": subdomain,
                "text": text,
                "output_content_type": "application/json",
            }),
            "output_format": "json",
        }

        return await self._make_request(f'{API_PREFIX}/zone/add_txt', params=params, timeout=120)
