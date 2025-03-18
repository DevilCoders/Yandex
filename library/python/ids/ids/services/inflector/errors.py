# coding: utf-8
from ids.exceptions import IDSException


class InflectorError(IDSException):
    """Базовая ошибка склонятора"""


class InflectorParseError(InflectorError):
    """Ошибка разбора ответа инфлектора"""


class InflectorBadFormatStringError(InflectorError):
    """Ошибка разбора строки форматирования"""


class InflectorBadArguments(InflectorError):
    """Ошибка разбора аргументов склонятора"""
