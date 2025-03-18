from .base import BaseClient
from ..auth_types import TVM2
from ..exceptions.base import BadResponseStatus
from ..exceptions.gendarme import DomainNameTooLongException, DomainNotFoundException
from typing import Dict
from aiohttp import FormData


class Client(BaseClient):
    """

    Сервис настроек почты
    https://wiki.yandex-team.ru/pochta/backend/yserversettings/
    """
    RESPONSE_TYPE = 'text'
    AUTH_TYPES = {TVM2, }

    async def update_profile(self, uid: str, data: Dict):
        return await self._make_request(
            path='update_profile',
            method='post',
            params={
                'uid': uid
            },
            data=FormData(data)
        )
