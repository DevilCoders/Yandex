# coding: utf-8

import six
import traceback
from abc import ABCMeta, abstractmethod

from tools_structured_logs.logic.configuration.library import get_config, get_logger
from tools_structured_logs.logic.state import get_state


class IRecorder(object):
    """
    Пишет в логи инфу, абстрагирован от вендоров.
    """
    __metaclass__ = ABCMeta

    state = None

    def __init__(self):
        """
        @type logger: Logger
        """
        self.state = get_state()

    @abstractmethod
    def provide_context(self, info_provider):
        """
        Предоставить контекст для записи.
        @rtype: dict
        @type info_provider: IVendorInfoProvider
        """

    def make_record(self, info_provider):
        """
        @type info_provider: IVendorInfoProvider
        """
        logger = get_logger()
        if not self.state.is_enabled():
            return

        profiling = self.provide_context(info_provider)
        profiling['vendor'] = info_provider.vendor()

        profiling['query_to_analyse'] = info_provider.query_to_analyse()

        if get_config().get_enable_stack_traces():
            profiling['stacktrace'] = ''.join(
                i.decode('utf-8') if six.PY2 else i
                for i in traceback.format_stack()[:-1]
            )

        params = {
            'profiling': profiling,
        }

        with logger.log_context(execution_time=int(info_provider.duration()), **params):
            logger.info('(%.1f msec) %s', info_provider.duration(), info_provider.main_readable_string())

        self.state.add_time(info_provider.vendor(), info_provider.duration())
