import socket

import aiohttp
from typing import Optional, NoReturn

from .base import BaseClient
from ..exceptions.passport import (
    PassportException,
    DomainAliasNotFound,
    DomainAlreadyExists,
    DomainAliasAlreadyExists,
    DomainNotFound,
)


class Client(BaseClient):
    AUTH_TYPES = tuple()

    _method_templates = {
        # Create Passport-account:
        # https://wiki.yandex-team.ru/passport/api/bundle/registration/pdd/
        'account_add': '/1/bundle/account/register/pdd/',

        # Edit Passport-account:
        # https://wiki.yandex-team.ru/passport/api/bundle/changeaccount/#izmenitpersonalnyedannyebandlovajaversija
        'account_edit': '/1/bundle/account/person/',

        # Delete Passport-account:
        # https://wiki.yandex-team.ru/passport/python/api/bundle/deleteaccount/
        'account_delete': '/1/bundle/account/{uid}/',

        # Добавляем домен в паспорте
        # https://wiki.yandex-team.ru/passport/api/bundle/mdapi/#sozdaniedomena
        'domain_add': '/1/bundle/pdd/domain/',

        # Изменяем свойства домена (пока изменяем только organization_name)
        # https://wiki.yandex-team.ru/passport/python/api/bundle/account/#izmeneniesvojjstvakkaunta
        'domain_edit': '/1/bundle/pdd/domain/{domain_id}/',

        # меняем мастер домен
        # https://wiki.yandex-team.ru/passport/api/bundle/mdapi/#obmendomenasodnimizegoaliasov
        'change_master_domain': '/1/bundle/pdd/domain/{old_master_id}/alias/{new_master_id}/make_master/',

        # https://wiki.yandex-team.ru/passport/api/bundle/account/flushpdd/
        'change_password_submit': '/1/account/%s/flush_pdd/submit/',

        # https://wiki.yandex-team.ru/passport/api/bundle/account/flushpdd/
        'change_password_commit': '/1/account/%s/flush_pdd/commit/',

        # https://wiki.yandex-team.ru/passport/api/bundle/mdapi/#sozdaniealiasadljapolzovatelja
        'add_alias': '/1/account/{uid}/alias/pddalias/{alias}/',

        # https://wiki.yandex-team.ru/passport/api/bundle/mdapi/#udaleniealiasapolzovatelja
        'delete_alias': '/1/account/{uid}/alias/pddalias/{alias}/',

        # https://st.yandex-team.ru/PASSP-30706
        'add_pdd_alias': '/1/account/{uid}/alias/pdddomain',

        # https://st.yandex-team.ru/PASSP-30706
        'delete_pdd_alias': '/1/account/{uid}/alias/pdddomain',

        # блокировка/разблокировка пользователя (меняем ему is_enabled)
        # https://wiki.yandex-team.ru/passport/api/bundle/changeaccount/#izmenitsvojjstvaakkaunta)
        'block_user': '/1/account/%s/',

        # Загрузка аватарки по UID
        # https://wiki.yandex-team.ru/passport/api/bundle/changeavatar/#zagruzkaavatarkipouidsessionnojjkukeilitokenubeztreka
        'change_avatar': '/2/change_avatar/',

        # Изменяем настройки аккаунта
        # https://wiki.yandex-team.ru/passport/api/bundle/changeaccount/#izmenitnastrojjkiakkauntazablokirovannostfaktsovmestnogoispolzovanijavkljuchjonnostparolejjprilozhenijjit.p
        'change_options': '/2/account/%s/options/',

        # Подписка аккаунтов на сиды.
        # Пока используется для того, чтобы "принимать" пользовательское
        # соглашение при создании роботных аккаунтов.
        #
        # Документация:
        # https://wiki.yandex-team.ru/passport/api/bundle/managesids/#sozdanieiliizmeneniepodpiski
        'subscription': '/1/account/{uid}/subscription/{service_slug}/',

        # Добавить домен как алиас
        # https://wiki.yandex-team.ru/passport/api/bundle/mdapi/#sozdaniealiasadljadomena
        'domain_alias_add': '/1/bundle/pdd/domain/{domain_id}/alias/',

        # Удалить алиас домена
        # https://wiki.yandex-team.ru/passport/api/bundle/mdapi/#udaleniealiasadomena
        'domain_alias_delete': '/1/bundle/pdd/domain/{domain_id}/alias/{alias_id}/',

        # Валидируем логины (label групп и отделов)
        # https://wiki.yandex-team.ru/passport/api/bundle/validate/#proveritloginbundleversija
        'validate_login': '/1/bundle/validate/login/',

        # Валидируем display name
        # https://wiki.yandex-team.ru/passport/api/bundle/validate/#validacijaimenifamiliiidisplayname
        'validate_display_name': '/1/bundle/validate/display_name/',

        # Валидируем firstname
        # https://wiki.yandex-team.ru/passport/api/bundle/validate/#validacijaimenifamiliiidisplayname
        'validate_firstname': '/1/bundle/validate/firstname/',

        # Валидируем lastname
        # https://wiki.yandex-team.ru/passport/api/bundle/validate/#validacijaimenifamiliiidisplayname
        'validate_lastname': '/1/bundle/validate/lastname/',

        # Валидируем пароль
        # https://wiki.yandex-team.ru/passport/api/bundle/validate/#proveritparolbandlovajaversija
        'validate_password': '/1/bundle/validate/password/',

        # Создаем трек
        # https://wiki.yandex-team.ru/passport/api/bundle/track/#sozdattrek
        'create_track': '/1/track/',

        # Валидируем натуральный домен в Паспорте
        # https://wiki.yandex-team.ru/passport/api/bundle/validate/#provalidirovatpdd-domen
        'validate_natural_domain': '/1/bundle/validate/domain/',

        # Валидируем коннектный домен в Паспорте
        # https://wiki.yandex-team.ru/passport/api/bundle/validate/#provalidirovatdomendirektorii
        'validate_connect_domain': '/1/bundle/validate/directory_domain/',

        # Удалить домен из паспортной базы
        # https://wiki.yandex-team.ru/passport/api/bundle/mdapi/#udaleniedomena
        'domain_delete': '/1/bundle/pdd/domain/{domain_id}/',
    }

    _exc_class_map = {
        'domain_alias.not_found': DomainAliasNotFound,
        'domain.already_exists': DomainAlreadyExists,
        'domain_alias.already_exists': DomainAliasAlreadyExists,
        'domain.not_found': DomainNotFound,
    }

    def __init__(self, *args, consumer_query_param: str, service_ticket: str = None, **kwargs):
        super().__init__(*args, **kwargs)
        self.consumer_query_param = consumer_query_param
        self.service_ticket = service_ticket

    def _prepare_params(self, params: Optional[dict]) -> Optional[dict]:
        params = params.copy() if params else {}
        params['consumer'] = self.consumer_query_param
        return params

    async def parse_response(self, response: aiohttp.ClientResponse, **kwargs):
        response = await super().parse_response(response)
        if response.get('status') == 'error':
            error_code = response['errors'][0]
            self._raise_for_error_code(error_code)

        return response

    def get_headers(self) -> dict:
        if self.service_ticket:
            return {'X-Ya-Service-Ticket': self.service_ticket}
        return {}

    @staticmethod
    def _get_localhost_ip_address() -> str:
        try:
            client_ip = socket.getaddrinfo(socket.getfqdn(), 80)[0][4][0]
        except socket.gaierror:
            client_ip = '127.0.0.1'

        return client_ip

    def get_auth_headers(self, use_type: Optional[str] = None) -> dict:
        return {
            'Ya-Client-Host': 'yandex.ru',
            'Ya-Consumer-Client-Ip': self._get_localhost_ip_address(),
            'Ya-Consumer-Client-Scheme': 'https',
        }

    def _raise_for_error_code(self, error_code: str) -> NoReturn:
        exc_class = self._exc_class_map.get(error_code, PassportException)
        raise exc_class(error_code)

    async def set_master_domain(self, old_master_id: int, new_master_id: int) -> bool:
        path = self._method_templates['change_master_domain'].format(
            old_master_id=old_master_id,
            new_master_id=new_master_id
        )
        response = await self._make_request(path=path, method='post')
        return response.get('status') == 'ok'

    async def domain_add(self, domain_name: str, admin_id: int) -> bool:
        path = self._method_templates['domain_add']
        response = await self._make_request(path=path, method='post', data={
            'domain': domain_name,
            'admin_uid': admin_id,
        })
        return response.get('status') == 'ok'

    async def domain_alias_add(self, domain_id: int, alias: str) -> bool:
        path = self._method_templates['domain_alias_add'].format(
            domain_id=domain_id)
        response = await self._make_request(path=path, method='post', data={
            'alias': alias,
        })
        return response.get('status') == 'ok'

    async def domain_alias_delete(self, domain_id: int, alias_id: int) -> bool:
        path = self._method_templates['domain_alias_delete'].format(
            domain_id=domain_id, alias_id=alias_id)
        try:
            response = await self._make_request(path=path, method='delete')
            return response.get('status') == 'ok'
        except DomainAliasNotFound:
            return True

    async def domain_edit(self, domain_id: int, data: dict) -> bool:
        path = self._method_templates['domain_edit'].format(
            domain_id=domain_id)
        response = await self._make_request(path=path, method='post', data=data)
        return response.get('status') == 'ok'

    async def domain_delete(self, domain_id: int) -> bool:
        path = self._method_templates['domain_delete'].format(
            domain_id=domain_id)
        try:
            response = await self._make_request(path=path, method='delete')
            return response.get('status') == 'ok'
        except DomainNotFound:
            return True

    async def validate_natural_domain(self, punycode_domain: str) -> bool:
        path = self._method_templates['validate_natural_domain']
        response = await self._make_request(path=path, method='post', data={
            'domain': punycode_domain,
        })
        return response.get('status') == 'ok'

    async def validate_connect_domain(self, punycode_domain: str) -> bool:
        path = self._method_templates['validate_connect_domain']
        response = await self._make_request(path=path, method='post', data={
            'domain': punycode_domain
        })
        return response.get('status') == 'ok'

    async def add_pdd_alias(self, uid: str, alias: str) -> bool:

        path = self._method_templates['add_pdd_alias'].format(uid=uid)
        response = await self._make_request(path=path, method='post', data={
            'alias': alias,
        })
        return response.get('status') == 'ok'

    async def delete_pdd_alias(self, uid: str) -> bool:

        path = self._method_templates['delete_pdd_alias'].format(uid=uid)
        response = await self._make_request(path=path, method='delete')
        return response.get('status') == 'ok'
