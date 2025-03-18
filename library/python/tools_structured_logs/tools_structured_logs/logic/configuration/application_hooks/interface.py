# coding: utf-8

from abc import ABCMeta, abstractmethod
from contextlib import contextmanager

from tools_structured_logs.logic.common_log_fields.composer import FieldComposer
from tools_structured_logs.logic.log_records.total_vendor_info import VendorInfoInHttpRequest


class InstrumentedApplicationHooks(object):
    __metaclass__ = ABCMeta
    logger = None

    def __init__(self, logger):
        """
        @type logger: Logger
        """
        self.logger = logger

    @abstractmethod
    def exec_on_request_started(self, any_func):
        """
        Выполнить когда стартует обработка HTTP-запроса
        """
        pass

    @contextmanager
    def all_the_logs_for_http(self, request, threshold=0, **kwargs):
        """
        Получить в логах и формат fields и информацию о подключенных вендорах.

        HTTP запрос в приложение должен быть обернут в этот контекстный менеджер.
        """
        kwargs = kwargs.copy()
        kwargs['request'] = request

        with self.logger.log_context(**FieldComposer().do(**kwargs)), VendorInfoInHttpRequest(request, threshold):
            yield
