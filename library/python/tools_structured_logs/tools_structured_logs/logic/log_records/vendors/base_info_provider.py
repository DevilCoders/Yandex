# coding: utf-8

from abc import ABCMeta, abstractmethod
from monotonic import monotonic

from tools_structured_logs.logic.configuration.library import get_logger
from tools_structured_logs.logic.errors import MissingContextError


class IVendorInfoProvider(object):
    __metaclass__ = ABCMeta
    _recorders = None
    context_requires = None

    def __init__(self):
        self._recorders = []
        self._init_recorders()
        self._context = None

    @abstractmethod
    def main_readable_string(self):
        """
        Что человек должен увидеть в первую очередь
        """

    @abstractmethod
    def query_to_analyse(self):
        """
        По этой строке будет строиться группировка в логах
        """

    @abstractmethod
    def supported_recorders(self):
        """
        Список классов, которые будут писать в логи используя этого провайдера.

        @rtype: List[IRecorder]
        """

    def _init_recorders(self):
        for recorder_class in self.supported_recorders():
            self._recorders.append(recorder_class())

    def duration(self):
        return self._context.setdefault(
            'duration',
            (monotonic() - self._context['started']) * 1000
        )

    def set_context(self, **context):
        """
        @rtype: IVendorInfoProvider
        """
        self._context = context
        return self

    def clear_context(self):
        self._context = None

    @abstractmethod
    def is_enabled(self):
        pass

    def make_records(self):
        self._check_context_is_enough_or_raise()
        if not self.is_enabled():
            get_logger().debug('"%s"" is disabled, will not produce', self.vendor())
        else:
            for recorder in self._recorders:
                recorder.make_record(self)
        self.clear_context()

    def _set_started_time(self, time):
        self._context['started'] = time

    def _check_context_is_enough_or_raise(self):
        if self.context_requires:
            difference = set(self.context_requires) - set(self._context.keys())
            if len(difference) > 0:
                raise MissingContextError(difference)

    def __enter__(self):
        self._set_started_time(monotonic())

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.make_records()

    @abstractmethod
    def vendor(self):
        """
        @rtype: str
        """
