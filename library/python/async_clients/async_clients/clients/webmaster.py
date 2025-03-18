from aiohttp import ClientResponse
from typing import List, Union, Optional, Iterable

from .base import BaseClient
from ..auth_types import TVM2
from ..exceptions.webmaster import (
    WebmasterException,
    UnAllowedVerificationType,
    DurationIsRequired,
)

# Полный список возможных типов можно посмотреть в документации:
# https://webmaster-internal.common.yandex.net/user/host/verification/verify.info#open(params-verificationType)
ALLOWED_VERIFICATION_TYPES = (
    'DNS',
    'HTML_FILE',
    'META_TAG',
    'WHOIS',
    'DNS_DELEGATION',
    # Этот способ не надо показывать обычным пользователям, он используется лишь
    # для ПДД регистраторов.
    #
    # Внутри он включает в себя две проверки:
    #
    # – CNAME с yamail-c269887f812cb586.mydomain.ru должен вести на mail.yandex.ru
    # - Файла на http://mydomain.ru/c269887f812cb586.html
    #
    # достаточно, чтобы сработала любая из них.
    'PDD_EMU',
)
PUBLIC_VERIFICATION_TYPES = set(ALLOWED_VERIFICATION_TYPES) - {'DNS_DELEGATION', 'PDD_EMU'}


class Client(BaseClient):
    AUTH_TYPES = {TVM2, }

    @staticmethod
    def get_errors_except_ignored(response: dict, ignore_errors: Optional[Union[Iterable, bool]] = None) -> List:
        errors = response.get('errors', [])
        if ignore_errors is not None:
            # Уберём ошибки, которые нужно игнорировать
            errors = [
                error for error in errors
                if error['code'] not in ignore_errors
            ]
        return errors

    async def parse_response(self, response: ClientResponse, **kwargs) -> dict:
        response = await super().parse_response(response)
        ignore_errors = kwargs.get('ignore_errors')

        if ignore_errors is not True:
            errors = self.get_errors_except_ignored(
                response=response,
                ignore_errors=ignore_errors,
            )

            if errors:
                # В исключении возвращаем код самой первой ошибки.
                first_error = errors[0]
                raise WebmasterException(
                    message=first_error.get('message') or 'No message',
                    code=first_error['code'],
                )
        return response

    async def add(self, domain: str, admin_uid: int) -> dict:
        """
        Добавляет домен в вебмастер
        """
        data = {
            'domain': domain,
            'userId': admin_uid,
        }
        return await self._make_request(
            path='/user/domain/verification/add.json',
            method='post',
            json=data,
            ignore_errors='ADD_HOST__HOST_ALREADY_ADDED',
        )

    async def add_registrar_domain(self, domain: str, admin_uid: int) -> None:
        """
        Добавляет домен от имени регистратора и запускает нужную проверку.
        """
        await self.add(domain=domain, admin_uid=admin_uid)
        await self.verify(
            domain=domain,
            admin_uid=admin_uid,
            verification_type='PDD_EMU',
        )

    async def lock_dns_delegation(self, domain: str, admin_uid: int, duration: int) -> dict:
        """
        Закрепляем домен за пользователем на определённый срок.
        В течение этого срока другие пользователи не смогут выбрать подтверждение хоста
        через делегирование DNS хостингу Яндекса.
        """
        data = {
            'domain': domain,
            'userId': admin_uid,
            'durationMinutes': duration / 60,
        }
        return await self._make_request(
            path='/user/host/verification/lockDnsDelegation.json',
            method='post',
            json=data,
        )

    async def verify(self, domain: str, admin_uid: int, verification_type: str, duration: Optional[int] = None) -> dict:
        # У вебмастера внутри все типы подтверждения написаны капслоком
        # поэтому прежде чем туда передавать что-то, надо способ подтверждения
        # привести к капслоку.
        original_verification_type = verification_type
        verification_type = verification_type.upper()

        # Проверим, что способ подтверждения указан правильно.
        if verification_type not in ALLOWED_VERIFICATION_TYPES:
            message = f'Verification type {original_verification_type} is not supported'
            raise UnAllowedVerificationType(message)

        # Если подтверждаем через делегирование, то сначала закрепляем домен за пользователем
        # на определённый срок
        if verification_type == 'DNS_DELEGATION':
            if duration is None:
                raise DurationIsRequired()
            await self.lock_dns_delegation(
                domain=domain,
                admin_uid=admin_uid,
                duration=duration,
            )

        data = {
            'domain': domain,
            'userId': admin_uid,
            'verificationType': verification_type,
        }
        return await self._make_request(
            path='/user/domain/verification/verify.json',
            method='post',
            json=data,
            ignore_errors=['VERIFY_HOST__ALREADY_VERIFIED'],
        )

    async def info(self, domain: str, admin_uid: int, ignore_errors: Optional[Union[Iterable, bool]] = None) -> dict:
        """
        Через ручку info получаем данные о состоянии подтверждения домена.
        """
        data = {
            'domain': domain,
            'userId': admin_uid,
        }
        return await self._make_request(
            path='/user/domain/verification/info.json',
            method='post',
            json=data,
            ignore_errors=ignore_errors,
        )

    async def is_verified(self, domain: str, admin_uid: int) -> bool:
        """
        Через ручку info проверяем, подтверждён ли домен в Вебмастере.
        Ручка должна вернуть данные со статусом VERIFIED или None, если домена нет в Вебмастере.
        """
        response = await self.info(
            domain=domain,
            admin_uid=admin_uid,
            ignore_errors=['USER__HOST_NOT_ADDED'],
        )
        data = response.get('data') or {}
        status = data.get('verificationStatus')
        return status == 'VERIFIED'

    async def list_applicable(self, domain, admin_uid, only_public=True, ignore_errors: Optional[Union[Iterable, bool]] = None) -> List[str]:
        """
        Возвращает имена методов подтверждения домена.
        """
        data = {
            'domain': domain,
            'userId': admin_uid,
        }
        response = await self._make_request(
            path='/user/domain/verification/listApplicable.json',
            method='post',
            json=data,
            ignore_errors=ignore_errors,
        )

        # Если домен не подтверждён, и проверка не запущена, то вебмастер отдаёт None
        # вместо словаря с данными
        data = response['data'] or {}
        methods = data.get('applicableVerifications', [])
        if only_public:
            methods = set(methods) & PUBLIC_VERIFICATION_TYPES
        return list(methods)

    async def reset(self, domain: str, for_user_id: int, user_id: int, ignore_errors: Optional[Union[Iterable, bool]] = None) -> dict:
        """
        Делает HTTP запрос в Вебмастер для перепроверки прав на домен для указанного пользователя.
        В случае, если права были делегированы другим пользователем - отменит делегирование
        """
        data = {
            'domain': domain,
            'forUserId': for_user_id,
            'userId': user_id,
        }
        return await self._make_request(
            path='/user/domain/verification/reset.json',
            method='post',
            json=data,
            ignore_errors=ignore_errors,
        )
