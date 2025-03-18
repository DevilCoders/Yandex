from typing import Dict, AnyStr, Optional
from dataclasses import dataclass, field

from tvm2.ticket import ServiceTicket, UserTicket


class BaseYandexUser:
    def is_authenticated(self) -> bool:
        raise NotImplementedError

    def authenticated_by(self) -> Optional[str]:
        backend = getattr(self, 'backend', None)
        if backend is not None:
            return backend.__class__.__module__.split('.')[-1]


@dataclass
class YandexUser(BaseYandexUser):
    """
    @param uid: UID пользователя
    @param auth_type: Тип аутентификации вида 'oauth'/'user_ticket'/...
    @param fields: значения запрошенных полей базы данных Паспорта
    @param blackbox_result: ответ от блэкбокса целиком
    @param backend: примененный механизм авторизации
    @param service_ticket: распрарсенный сервисный tvm-ticket
    @param user_ticket: распрарсенный персонализированный tvm-ticket
    @param raw_user_ticket: персонализированный tvm-ticket
    @param raw_service_ticket: сервисный tvm-ticket
    @param user_ip: user ip из запроса
    """
    uid: AnyStr
    backend: 'BaseBackend'
    auth_type: str = None
    fields: Dict = field(default_factory=dict)
    blackbox_result: Dict = None
    service_ticket: ServiceTicket = None
    user_ticket: UserTicket = None
    raw_service_ticket: AnyStr = None
    raw_user_ticket: AnyStr = None
    user_ip: AnyStr = None

    def __post_init__(self):
        if self.auth_type is None:
            self.auth_type = self.authenticated_by()

    def is_authenticated(self):
        return True


@dataclass
class AnonymousYandexUser(BaseYandexUser):
    """
    @param backend: примененный механизм авторизации
    @param reason: текстовая причина неуспешной авторизации
    """
    backend: 'BaseBackend' = None
    reason: AnyStr = None

    def is_authenticated(self):
        return False
