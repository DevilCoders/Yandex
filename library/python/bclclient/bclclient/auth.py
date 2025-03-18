from typing import Tuple, Union

from tvm2 import TVM2
from tvm2.protocol import BlackboxClientId

from .exceptions import BclClientException

AuthArg = Union[Tuple[str, str], 'TvmAuth']


class TvmAuth:
    """Обеспечивает доступ к BCL API при помощи сервисных TVM-билетов."""

    def __init__(self, *, app_id: str, secret: str):
        """

        :param app_id: ID TVM клиентского приложения.
        :param secret: Секрет этого приложения.

        """
        self.tvm = TVM2(
            client_id=app_id,
            secret=secret,
            blackbox_client=BlackboxClientId.ProdYateam,
        )
        self.client_id = app_id
        self.bcl_id: str = ''

    def get_ticket(self) -> str:
        """Возвращает билет для обращения клиентского приложения к BCL."""

        app_id = self.bcl_id
        tickets = self.tvm.get_service_tickets(app_id)

        ticket = tickets.get(app_id) or ''

        if not ticket:
            raise BclClientException(f'Unable to get TVM ticket for {self.client_id} -> {app_id}')

        return ticket

    @classmethod
    def from_arg(cls, auth_data: AuthArg, bcl_app_id: str = None) -> 'TvmAuth':
        """Альтернативный конструктор.
        Возвращает объект класса, ориентируясь на указаные аргументы.

        :param auth_data: Данные для авторизации:
            * Объект TvmAuth
            * кортеж (id_приложения, секрет_приложения)

        :param bcl_app_id: Идентификатор TVM приложения BCL, к которому
            будут осуществляться подключения.

        """
        if isinstance(auth_data, TvmAuth):
            auth = auth_data

        else:
            auth = TvmAuth(app_id=auth_data[0], secret=auth_data[1])

        if bcl_app_id:
            auth.tvm.add_destinations(bcl_app_id)
            auth.bcl_id = bcl_app_id

        return auth
