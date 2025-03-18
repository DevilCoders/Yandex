# coding: utf-8

from .base import StructuredLogsError


__all__ = [
    'MissingContextError',
    'NoConfigureTwiceError',
    'ConfigureFirstError',
]


class MissingContextError(StructuredLogsError):
    _missing = None

    def __init__(self, missing, *args, **kwargs):
        """

        @type missing: iterable
        """
        self._missing = missing
        super(MissingContextError, self).__init__(
            'Context is missing: {}'.format(
                ', '.join(missing)
            )
        )


class NoConfigureTwiceError(StructuredLogsError):
    """
    Нельзя дважды конфигурировать библиотеку.
    """


class ConfigureFirstError(StructuredLogsError):
    """
    Сначала надо сконфигурировать библиотеку.
    """
