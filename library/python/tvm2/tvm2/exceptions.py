# coding: utf-8

from __future__ import unicode_literals


class TVM2BaseException(Exception):
    pass


class NotAllRequiredKwargsPassed(TVM2BaseException):
    """
    При инициализации не были переданы все
    требуемые аргументы
    """
    pass
