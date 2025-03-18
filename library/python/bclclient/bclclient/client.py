from typing import Type

from .auth import AuthArg
from .http import Connector
from .resources import Utils, Reference, Payments, Proxy, Statements


class Bcl:
    """Предоставляет интерфейс для взаимодествия с BCL."""

    cls_connector: Type[Connector] = Connector

    def __init__(self, *, auth: AuthArg, host: str = None, timeout: int = None):
        """
        :param auth: Данные для авторизации:
            * Объект TvmAuth
            * кортеж (id_приложения, секрет_приложения)

        :param host: Имя хоста, на котором находится BCL. Будет использован протокол HTTPS.
            Если не указан, будет использован хост по умолчанию (см. .settings.HOST_DEFAULT).

        :param int timeout: Таймаут на подключение. По умолчанию: 5 сек.

        """
        connector = self.cls_connector(auth=auth, host=host, timeout=timeout)
        self._connector = connector

        self.payments = Payments(connector)
        """Инструменты для работы с платежами."""

        self.proxy = Proxy(connector)
        """Инструменты для проброса запросов во внешние системы."""

        self.reference = Reference(connector)
        """Базовые справочники."""

        self.utils = Utils(connector)
        """Утилиты."""

        self.statements = Statements(connector)
        """Выписки."""
