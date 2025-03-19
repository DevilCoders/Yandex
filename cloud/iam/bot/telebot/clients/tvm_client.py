from requests import Session
from tvm2 import TVM2

from cloud.iam.bot.telebot.clients import BaseClient


# применить decorator ко всем методам базового класса cls, входящим в should_be_decorated
def for_all_methods(should_be_decorated, decorator):
    def decorate(cls):
        for attr in cls.__base__.__dict__:
            if callable(getattr(cls, attr)) and attr in should_be_decorated:
                setattr(cls, attr, decorator(getattr(cls, attr)))
        return cls
    return decorate


def add_ticket_decorator(method_function):
    def wrapper(self, *args, **kwargs):
        self.add_ticket(kwargs)
        return method_function(self, *args, **kwargs)

    return wrapper


@for_all_methods(frozenset(['get', 'options', 'head', 'post', 'put', 'patch', 'delete']),
                 add_ticket_decorator)
class TvmSession(Session):
    __tvm_headers_key = 'X-Ya-Service-Ticket'

    def __init__(self, application_id: str, tvm: TVM2):
        super().__init__()
        self.__application_id = application_id
        self.__tvm = tvm

    def add_ticket(self, kwargs_dict) -> None:
        ticket = self.__tvm.get_service_ticket(self.__application_id)

        if 'headers' in kwargs_dict:
            kwargs_dict[TvmSession.__tvm_headers_key] = ticket
        else:
            kwargs_dict['headers'] = {TvmSession.__tvm_headers_key: ticket}


class TvmClient(BaseClient):
    application_id: str = 'should be set'

    def __init__(self, config, tvm: TVM2):
        super().__init__(config)
        self.__tvm = tvm

    def _create_session(self):
        session = TvmSession(self.application_id, self.__tvm)
        self._mount_session(session)
        return session
