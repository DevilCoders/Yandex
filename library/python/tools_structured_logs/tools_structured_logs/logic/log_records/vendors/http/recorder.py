# coding: utf-8


from tools_structured_logs.logic.log_records.vendors.base_recorder import IRecorder


class HttpRecorder(IRecorder):
    """
    Пишет в логи инфу про http запросы, которое сделало ваше приложение.
    """

    def provide_context(self, info_provider):
        """
        @type info_provider: HttpRequestProvider
        """
        return {
            'method': info_provider.get_http_method(),
            'status_code': info_provider.get_status_code(),
            'hostname': info_provider.get_hostname(),
            'path': info_provider.get_path(),
            'query': info_provider.get_query(),
            'content': None,
        }
