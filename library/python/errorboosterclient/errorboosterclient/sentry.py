# -*- coding: utf-8 -*-
"""
Предоставляет инструменты для отправки данных в ErrorBooster через интерфейс sentry-sdk.
https://clubs.at.yandex-team.ru/python/3274

"""
from __future__ import unicode_literals

import logging
import traceback
from collections import namedtuple
from typing import Callable, Union, Type
from typing import Iterable

import sys
from itertools import chain
from sentry_sdk import Transport, Hub
from sentry_sdk import capture_exception
from sentry_sdk.utils import capture_internal_exceptions
from sentry_sdk.worker import BackgroundWorker
from time import time


class EventConverter(object):
    """Преобразует данные события из формата Sentry в формат ErrorBooster."""

    keys = {
        'tags': (
            'dc',
            # Датацентр [vla, man, ...].
            'reqid',
            # ИД запроса requestId.
            'platform',
            # desktop|app|tv|tvapp|station|unsupported;
            #   псевдонимы: touch - touch|touch-phone|phone; pad - pad|touch-pad
            'isInternal',
            # Ошибка внутри сети.
            'isRobot',
            # Ошибка при визите робота.
            'block',
            # Блок или модуль, в котором произошла ошибка, например: stream.
            'service',
            # Если на одной странице живет несколько сервисов, например: Эфир, Дзен на морде.
            'source',
            # Источник ошибки, например: ugc_backend.
            'sourceMethod',
            # Метод источника, например: set_reactions.
            'sourceType',
            # Тип ошибки, например: network, logic.
            'url',
            # URL (полный, со схемой и доменом), на котором возникла ошибка.
            'page',
            # Тип (семейство) страницы, например: главная, корзина, товар, список в категории и тд.
            'region',
            # ID региона из геобазы.
            'slots',
            # Слоты экспериментов в формате
            # https://beta.wiki.yandex-team.ru/JandeksPoisk/combo/logs/#projeksperimenty
            'experiments',
            # Эксперименты, при которых произошло событие.
            # Формат: "<exp_description1>;<exp_description2>;..." (разбивается по ";")
            # Например: aaa=1;bb=7;ccc=yes;ddd
            'useragent',
        ),
        'additional': (
            # Данные события сентри, попадающие в блок additional
            'contexts',
            'modules',
            'extra',
            'breadcrumbs',
        ),
    }

    MASK = '*' * 5
    MASK_NAMES = frozenset([
        'passwd',
        'pin',
        'passphrase',
        'password',
        'api_key',
        'apikey',
        'secret',
        'auth',
        'token',
        'credential',
        'sessionid',
    ])

    @classmethod
    def enrich(cls, source, destination, supported_keys):
        # type: (dict, dict, Iterable[str]) -> None

        if not source:
            return

        for key in supported_keys:
            value = source.get(key)
            if value:
                destination[key] = value

    @classmethod
    def sanitize_vars(cls, variables, check_values=False):
        # type: (dict, bool) -> dict
        """Производит санацию (чистку) переменных от чувствительных данных.
        Подобная чистка была в Raven, но из Sentry SDK её исключили, поэтому реализуем сами.

        Реализация в Raven:
            https://github.com/getsentry/raven-python/blob/master/raven/processors.py

        :param variables: Словарь с переменными, который требуется вычистить.
            Внимание: объект словаря модифицируется на месте (возвращается лишь для удобства).

        :param check_values: Следует ли помимо имён проверять значения переменных.
            Данная проверка затратна, поэтому включение вынесено флагом.

        """
        mask = cls.MASK
        mask_names = cls.MASK_NAMES
        sanitize = cls.sanitize_vars

        for key, val in variables.items():
            masked = False

            val_is_dict = isinstance(val, dict)
            val_is_list = isinstance(val, list)
            val_is_container = val_is_dict or val_is_list
            str_value = None

            for name in mask_names:
                if name in key:
                    masked = True
                    break

                if check_values and not val_is_container:
                    if str_value is None:
                        try:
                            str_value = str(val)
                        except UnicodeEncodeError:
                            # PY2 strings specific
                            str_value = str(val.encode('ascii', 'replace'))
                    if name in str_value:
                        masked = True
                        break

            if masked:
                variables[key] = mask

            elif val_is_dict:
                sanitize(val, check_values=check_values)

            elif val_is_list:

                for item in val:
                    if isinstance(item, dict):
                        sanitize(item, check_values=check_values)

        return variables

    @classmethod
    def convert(cls, event, sanitize_values=False):
        # type: (dict, bool) -> dict

        keys = cls.keys
        enrich = cls.enrich

        variables = []
        additional = {
            'eventid': event['event_id'],
            # уникальный идентификатор события для его адресации

            'vars': variables,
            # переменные во фреймах
        }
        enrich(source=event, destination=additional, supported_keys=keys['additional'])

        result = {
            'timestamp': event['timestamp_'],
            # штамп возникновения события

            'level': event['level'],
            # уровень ошибки trace|debug|info|error;
            # алиасы warning - [warn|warning]; critical - [critical|fatal]

            'env': event.get('environment', ''),
            # окружение development|testing|prestable|production|pre-production

            'version': event.get('release', ''),
            # версия приложения

            'host': event['server_name'],
            # хост источника ошибки

            'language': 'python',
            # nodejs|go|python

            'additional': additional,
            # произвольный набор полей вида key:string = value:string|object
            # в базе будет хранится как string:string(stringify).
            # Null-значения сюда передавать нельзя!
        }

        user = event.get('user', {})
        if user:
            ip = user.get('ip_address', '')
            if ip:
                result['ip'] = ip  # адрес клиента

            yuid = user.get('yandexuid', '')
            if yuid:
                result['yandexuid'] = yuid  # уникальный ид пользователя Яндекса

            puid = user.get('puid', '')
            if puid:
                additional['puid'] = puid  # уникальный ид аккаунта в Яндекс.Паспорте

        enrich(source=event.get('tags', {}), destination=result, supported_keys=keys['tags'])

        # заполнено, если ивент сгенерирован из capture_message
        message = event.get('message')

        logentry = event.get('logentry')

        # если ивент сгенерирован из строчки лога
        if logentry:
            message = logentry['message']  # строчка лога без параметров
            params = logentry['params']  # параметры (list или dict)
            if not params:
                additional['message'] = message
            else:
                if isinstance(params, list):
                    params = tuple(params)
                additional['message'] = message % params

        sanitize = cls.sanitize_vars

        # Санируем цепочку событий (хлебные крошки).
        breadcrumbs = additional.get('breadcrumbs', [])
        if breadcrumbs:

            if isinstance(breadcrumbs, dict):
                # Новые версии SDK используют словарь.
                breadcrumbs = breadcrumbs.get('values', [])

            for breadcrumb in breadcrumbs:
                sanitize(breadcrumb, check_values=True)

            additional['breadcrumbs'] = breadcrumbs

        exceptions = event.get('exception', {})

        if exceptions:
            stack = []
            # Разобранную клиентом сентри трассировку придётся вновь собрать в строку,
            # чтобы её мог переварить бустер.

            for exception in exceptions['values']:
                trace = exception['stacktrace']
                module = exception['module'] or ''

                if module:
                    module = '%s.' % module

                exc_message = '%s%s: %s' % (module, exception['type'], exception['value'])

                for frame in trace['frames'] or []:
                    path = frame['abs_path']
                    func = frame['function']
                    lineno = frame['lineno']

                    stack.append(
                        '  File "%s", line %s, in %s\n    %s' % (
                            path,
                            lineno,
                            func,
                            (frame.get('context_line') or '').strip(),
                        )
                    )
                    result.update({'file': path, 'method': func})

                    # Пробрасываем вычищенные локальные переменные.
                    variables.append({
                        'loc': '%s:%s %s()' % (path, lineno, func),
                        'vars': sanitize(frame.get('vars', {}), check_values=sanitize_values),
                    })

            if stack:
                result['stack'] = '\n'.join(
                    chain(['Traceback (most recent call last):'], stack, [exc_message])  # noqa
                )
                if not message:
                    message = exc_message

        # текст ошибки, все сэмплы ошибок будут сгруппированы по этому названию
        result['message'] = message
        return result


class ErrorBoosterTransport(Transport):
    """Псевдотранспорт (конвертер + транспорт), позволяющий отправлять
    события в ErrorBooster, используя машинерию клиента Sentry.


    .. code-block:: python

        def send_me(event: dict):
            ...

        sentry_sdk.init(
            transport=ErrorBoosterTransport(
                project='myproject',
                sender=send_me,
            ),
            environment='development',
            release='0.0.1',
        )

        with sentry_sdk.configure_scope() as scope:

            scope.user = {'ip_address': '127.0.0.1', 'yandexuid': '1234567'}
            scope.set_tag('source', 'backend1')

            try:
                import json
                json.loads('bogus')

            except:
                sentry_sdk.capture_exception()

    """
    converter_cls = EventConverter

    def __init__(self, project, sender, bgworker=True, options=None, sanitize_values=False):
        # type: (str, Callable, bool, None, bool) -> None
        """

        :param project: Название проекта, для которого записываем события.

        :param sender: Объект, поддерживающий вызов, осуществляющий отправку сообщения события.
            Должен принимать объект сообщения (словарь).

        :param bgworker: Следует ли использовать отдельную фоновую нить для отправки.
            Стоит помнить, что у .init() есть параметр shutdown_timeout, который
            может убивать нить.

        :param sanitize_values: Следует ли при санации переменных во фреймах учитывать
            наличие стоп-слов (см. EventConverter.MASK_NAMES) не только в их именах, но и в значениях.
            Позволяет маскировать реквизиты (секреты, пароли) в значениях ценой потенциального
            снижения производительности.

        :param options:

        """
        super(ErrorBoosterTransport, self).__init__(options)

        self.project = project
        self.sender = sender
        self.sanitize_values = sanitize_values

        worker = None

        if bgworker:
            worker = BackgroundWorker()

        self._worker = worker

        self.hub_cls = Hub

    def _convert(self, event):
        # type: (dict) -> dict
        converted = self.converter_cls.convert(event, sanitize_values=self.sanitize_values)
        converted['project'] = self.project
        return converted

    def _send_event(self, event):
        # type: (dict) -> None
        self.sender(self._convert(event))

    def capture_event(self, event):
        # type: (dict) -> None

        event['timestamp_'] = int(round(time() * 1000))  # бустер ожидает миллисекунды
        hub = self.hub_cls.current

        def send_event_wrapper():
            with hub:
                with capture_internal_exceptions():
                    self._send_event(event)

        worker = self._worker

        if worker:
            worker.submit(send_event_wrapper)

        else:
            send_event_wrapper()

    def capture_envelope(self, envelope):
        # Эта умолчательная реализация была убрана из базового Transport в 0.17.7
        # https://github.com/getsentry/sentry-python/commit/93f6d33889f3cc51181cb395f339b0672b1c080a
        # Мы возращаем её для совместимости с новыми версиями Sentry.
        event = envelope.get_event()
        if event is not None:
            self.capture_event(event)
        return None

    def flush(self, timeout, callback=None):
        worker = self._worker
        if worker and timeout > 0:
            worker.flush(timeout, callback)

    def kill(self):
        worker = self._worker
        worker and worker.kill()


ExceptionCaptureResult = namedtuple('ExceptionCaptureResult', ['uid', 'msg', 'trace'])
"""Результат захвата исключения."""


class ExceptionCapturer(object):
    """Захватчик исключений.
    Позволяет в ручном режиме захватывать исключения для отправки в ErrorBooster.

    """
    capture_expose_trace = False  # type: bool
    """Следует ли светить трассировку в результате захвата (полезно при отладке и в тестовой среде).
    Например, можно выставить в django.conf.settings.IN_PRODUCTION или нечто подобное.

    """

    log = logging.getLogger(__name__)  # type: logging.Logger
    """Журнал, который следует писать захватчику."""

    raise_capture_exc_cls = RuntimeError  # type: Type[Exception]
    """Тип исключения, которое следует поднять для захвата.
    Умолчательный, используется, если не указано иное в агрументе функции.

    """

    @classmethod
    def capture(cls):
        # type: () ->  ExceptionCaptureResult
        """Захватывает текущее исключение и возвращает идентификатор или текстовое представление."""

        uid = capture_exception()

        msg = ''
        trace = ''

        if uid:
            cls.log.exception('Captured exception %s', uid)

        else:
            # Сентри-клиент не стал звахатывать исключение и не сформировал uid.
            err_type, err_val, err_trace = sys.exc_info()
            msg = '%s: %s' % (err_type.__name__, err_val)

            if cls.capture_expose_trace:
                trace = traceback.format_exc()

            cls.log.exception('Uncaptured exception')

        return ExceptionCaptureResult(uid=uid, msg=msg, trace=trace)

    @classmethod
    def raise_capture(
        cls,
        data,
        exception_cls=None
    ):
        # type: (Union[Exception, str], Type[Exception])-> ExceptionCaptureResult
        """Поднимает исключение и инициирует его отправку в ErrorBooster.

        :param data: Строка описывающая событие, или объект исклчюения.

        :param exception_cls: Класс исключения.
            Используется в случае передачи строки в первом параметре.

        """
        if isinstance(data, Exception):
            exception_cls = data.__class__
            exception = data

        else:
            exception_cls = exception_cls or cls.raise_capture_exc_cls
            exception = exception_cls(data)

        # Чтобы задействовать общий механизм отправки, поднимаем исключение.
        try:
            raise exception

        except exception_cls:
            return cls.capture()
