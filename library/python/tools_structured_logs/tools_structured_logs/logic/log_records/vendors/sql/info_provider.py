# coding: utf-8

from abc import ABCMeta, abstractmethod

from tools_structured_logs.logic.log_records.vendors.base_info_provider import IVendorInfoProvider
from tools_structured_logs.logic.configuration.library import get_config
from .recorder import SqlRecorder


class Cursor(IVendorInfoProvider):
    __metaclass__ = ABCMeta

    @abstractmethod
    def query_prepared_sql(self):
        """
        Вернуть сырой SQL запрос без подставленных параметров.
        :return:
        """

    @abstractmethod
    def query_final_sql(self):
        """
        Вернуть итоговый SQL запрос с параметрами
        """

    @abstractmethod
    def query_transaction_isolation_level(self):
        """
        Уровень изоляции текущей транзакции.
        """

    @abstractmethod
    def query_transaction_status(self):
        """
        Статус транзакции

        @rtype: int
        """

    @abstractmethod
    def query_connection_encoding(self):
        """
        Кодировка соединения.
        """

    def query_to_analyse(self):
        return self.query_prepared_sql()

    def main_readable_string(self):
        return self.query_final_sql()

    def supported_recorders(self):
        return [
            SqlRecorder,
        ]

    def is_enabled(self):
        return get_config().get_enable_db_tracking()
