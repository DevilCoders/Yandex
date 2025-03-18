from typing import Optional
from abc import ABC, abstractmethod


class BaseAuthType(ABC):
    @abstractmethod
    def as_headers(self) -> dict:
        ...

    @classmethod
    def name(cls) -> str:
        return cls.__name__.lower()


class TVM2(BaseAuthType):

    def __init__(
            self,
            service_ticket: str,
            user_ticket: Optional[str] = None,
            **kwargs
    ):
        self.service_ticket = service_ticket
        self.user_ticket = user_ticket

    def as_headers(self) -> dict:
        result = {
            'X-Ya-Service-Ticket': self.service_ticket,
        }
        if self.user_ticket:
            result['X-Ya-User-Ticket'] = self.user_ticket

        return result


class OAuth(BaseAuthType):
    def __init__(
            self,
            token: str,
            **kwargs
    ):
        self.token = token

    def as_headers(self) -> dict:
        return {
            'Authorization': f'OAuth {self.token}'
        }


AUTH_TYPES_MAP = {
    cls.name(): cls
    for cls in BaseAuthType.__subclasses__()
}
