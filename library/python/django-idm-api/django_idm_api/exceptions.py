# coding: utf-8
from __future__ import unicode_literals


class BadRequest(RuntimeError):
    """Это исключение нужно выкидывать в случае, если были переданы неверные данные."""


class AccessDenied(RuntimeError):
    """Это исключение кидаем, когда по тем или иным причинам доступ к странице запрещён."""


class UserNotFound(BadRequest):
    """Пользователь не найден в базе данных"""


class GroupNotFound(BadRequest):
    """Группа не найдена в базе данных"""


class RoleNotFound(BadRequest):
    """Система не знает о таком узле дерева ролей"""


class UnsupportedApi(BadRequest):
    """Система не реализовала данный метод API"""


class TvmAppNotFound(BadRequest):
    """Tvm-приложение не найдено в базе данных"""


class TvmAppGroupNotFound(BadRequest):
    """Группа tvm-приложений не найдена в базе данных"""
