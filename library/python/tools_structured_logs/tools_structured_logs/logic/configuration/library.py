# coding: utf-8

__all__ = [
    'get_library',
    'get_config',
    'get_logger',
]

from tools_structured_logs.logic.state import reset_state
from tools_structured_logs.logic.errors import (
    NoConfigureTwiceError,
    ConfigureFirstError,
)


class Library(object):
    """
    Абстрактный протокол работы с библиотекой, абстрагированный от фреймворков.
    """
    __config = None
    instrumented_app_signals = None
    __logger = None

    __configured = False

    def __new__(cls):
        """
        Singletone
        """
        if not hasattr(cls, '_library_instance'):
            cls._library_instance = super(Library, cls).__new__(cls)
        return cls._library_instance

    # do not use __init__

    def configure(self, config, logger, instrumented_app_hooks, domain_models=None):
        """

        @type config: Config
        @type logger: Logger
        @type instrumented_app_hooks: InstrumentedApplicationHooks
        @type domain_models: List[IVendorInstrumenter]
        """
        if self.__configured:
            raise NoConfigureTwiceError
        self.instrumented_app_signals = instrumented_app_hooks
        if not config.get_enable_tracking():
            return
        for domain_model in domain_models:
            domain_model.instrument()
        self.instrumented_app_signals.exec_on_request_started(reset_state)
        self.__config = config
        self.__logger = logger
        self.__configured = True

    @property
    def logger(self):
        if not self.__configured:
            raise ConfigureFirstError
        return self.__logger

    @property
    def config(self):
        if not self.__configured:
            raise ConfigureFirstError
        return self.__config


def get_library(config, logger, instrumented_app_hooks, domain_models):
    """
    Сконфигурировать библиотеку.
    """
    instance = Library()
    instance.configure(
        config, logger,
        instrumented_app_hooks=instrumented_app_hooks,
        domain_models=domain_models
    )


def get_config():
    """
    Получить конфигурацию библиотеки.
    :return:
    """
    return Library().config


def get_logger():
    """
    Получить логгер.

    @rtype: Logger
    """
    return Library().logger
