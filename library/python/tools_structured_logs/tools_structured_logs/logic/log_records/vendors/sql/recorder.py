# coding: utf-8

from tools_structured_logs.logic.log_records.vendors.base_recorder import IRecorder
from tools_structured_logs.logic.configuration.library import get_config


class SqlRecorder(IRecorder):
    """
    Пишет в логи инфу про SQL запросы, которое сделало ваше приложение.
    """

    def provide_context(self, info_provider):
        """
        @type info_provider: Cursor
        """
        return {
            'is_select': info_provider.query_prepared_sql().strip().lower().startswith('select'),
            'is_slow': info_provider.duration() > get_config().get_sql_warning_threshold(),
            'vendor': 'database',
            'encoding': info_provider.query_connection_encoding(),
            'iso_level': info_provider.query_transaction_isolation_level(),
            'trans_status': info_provider.query_transaction_status(),
        }
