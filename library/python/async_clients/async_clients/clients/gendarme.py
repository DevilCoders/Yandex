from .base import BaseClient
from ..auth_types import TVM2
from ..exceptions.base import BadResponseStatus
from ..exceptions.gendarme import DomainNameTooLongException, DomainNotFoundException


def to_punycode(text):
    try:
        return text.encode('idna').decode()
    except UnicodeError as e:
        if 'too long' in str(e):
            message = 'Domain name {0} too long'.format(text)
            raise DomainNameTooLongException(message)
        raise


class Client(BaseClient):
    """
    Возвращает информацию про домен из сервиса Жандарм."
    https://wiki.yandex-team.ru/mail/pdd/gendarme/
    """

    AUTH_TYPES = {TVM2, }

    async def status(self, domain):
        try:
            return await self._make_request(
                path='/domain/status',
                method='get',
                params={
                    'name': to_punycode(domain)
                }
            )
        except BadResponseStatus as e:
            if getattr(e, 'status') == 404:
                raise DomainNotFoundException()
            raise

    async def recheck(self, domain, sync=False):
        return await self._make_request(
            path='/domain/recheck',
            method='post',
            json={
                'name': to_punycode(domain),
                'sync': sync,
            }
        )
