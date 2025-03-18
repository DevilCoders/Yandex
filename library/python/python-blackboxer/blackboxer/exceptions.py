# encoding: utf-8
from __future__ import print_function, unicode_literals


class BlackboxError(Exception):
    """ Базовое исключение """


class TransportError(BlackboxError):
    """ Не смогли получить ответ от блэкбокса """


class ConnectionError(TransportError):
    """ Ошибки соединения с сервером ЧЯ """

    def __init__(self, exc, *args, **kwargs):
        self.exc = exc

        super(ConnectionError, self).__init__(*args, **kwargs)


class HTTPError(TransportError):
    """ Ошибка в HTTP ответе от ЧЯ, например http код 400 """

    def __init__(self, response, *args, **kwargs):
        self.response = response

        super(HTTPError, self).__init__(*args, **kwargs)

    def __str__(self):
        return self.response


class FieldRequiredError(BlackboxError):
    """ Обязательное поле не установлено """


class ResponseError(BlackboxError):
    """ Ошибка ответа от ЧЯ """

    def __init__(self, id, value, error, *args, **kwargs):
        """
        http://doc.yandex-team.ru/blackbox/concepts/blackboxErrors.xml
        id - response['exception']['id'] идентификатор ошибки
        value - response['exception']['value'] текстовый идентификатор ошибки
        error - response['error'] текст ошибки для записи в лог
        """
        self.id = id
        self.value = value
        self.error = error

        super(ResponseError, self).__init__(*args, **kwargs)

    def __str__(self):
        return "<%s>" % self.error


class AccessDenied(ResponseError):
    """ Отсутствуют гранты на метод, параметр метода или на запрашиваемое поле в аргументе dbfields. """


class TemporaryError(ResponseError):
    """ Временная ошибка, желательно повторить запрос """


class InvalidParamsError(ResponseError):
    """ Неправильно указан или отсутствует обязательный аргумент. """


class UnknownError(ResponseError):
    """ Прочие ошибки. """
