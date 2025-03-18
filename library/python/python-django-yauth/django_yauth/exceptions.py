# -*- coding: utf-8 -*-


class AuthException(Exception):
    """
    Базовый класс для исключений аутентификации.
    """
    pass


class TwoCookiesRequired(AuthException):
    """
    Одна из двух кук является невалидной и требуется повторная авторизация для ее обновления.
    """
    pass


class AuthRequired(AuthException):
    """
    Эксепшн бросается в пользовательском коде, чтобы
    инициировать редирект на паспорт в middleware.
    """
    def __init__(self, retpath='', *args, **kwargs):
        self.retpath = retpath
        super(AuthRequired, self).__init__(*args, **kwargs)


class InvalidProtocol(AuthException):
    """
    Для корректной аутентификации в данном случае требуется соединение по HTTPS.
    """
    pass


class NoAuthMechanismError(AuthException):
    """
    Невозможно импортировать механизм аутентификации
    """
    pass
