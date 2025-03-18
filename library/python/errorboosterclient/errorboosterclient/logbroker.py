# -*- coding: utf-8 -*-
"""
Здесь расположен код, минимально необходимый для отправки данных в Logbroker.

ВНИМАНИЕ: прежде чем использовать, убедитесь, что у вас нет альтернатив,
в частности, возможности отправки в Unified Agent.

* Зависит от kikimr/public/sdk/python/persqueue (PyPI: ydb-persqueue; ранее kikimrclient)

Заполните переменные окружения:

  * LOGBROKER_TOKEN - токен для доступа к lb,
      выданный на пользователя, у которого есть доступ на запись в раздел.


Пример конфигурирования для sentry_sdk и Django в Deploy:

.. code-block:: python

    from django.core.serializers.json import DjangoJSONEncoder
    from errorboosterclient.logbroker import LogbrokerClient
    from errorboosterclient.sentry import ErrorBoosterTransport
    from sentry_sdk.integrations.django import DjangoIntegration

    NODE_DC = environ.get('DEPLOY_NODE_DC', '')
    TOPIC = 'bcl/test/trash'

    ERR_LOGBROKER_PRODUCER = LogbrokerClient().get_producer(
        source=f"{NODE_DC or 'dev'}_errboo",
        topic=TOPIC,
        json_encoder=DjangoJSONEncoder
    )

    sentry_sdk.init(
        transport=ErrorBoosterTransport(
            project='myproject',
            sender=ERR_LOGBROKER_PRODUCER.write
        ),
        integrations=[DjangoIntegration()],
        send_default_pii=True,

        environment='testing',
        release='1.0.0',

        shutdown_timeout=5,
    )

"""
import json
from os import environ
from typing import Union, Optional, Type

import kikimr.public.sdk.python.persqueue.auth as auth
import kikimr.public.sdk.python.persqueue.grpc_pq_streaming_api as pqlib
from kikimr.public.sdk.python.persqueue import errors


class LogbrokerException(Exception):  # pragma: nocover
    """Базовое исключение Logbroker-клиента."""

    def __init__(self, message, status=''):
        # type: (str, str) -> None
        self.message = message
        self.status = status


class LogbrokerClient(object):
    """Минимальный клиент для логброкера.

    with LogbrokerClient() as logbroker:
        ...

    """
    timeout = 15  # type: int

    def __init__(self, token='', endpoint='', timeout=None):
        # type: (str, str, int) -> None
        """

        :param token: Токен доступа.
            Если не передан, будет взят из переменной окружения LOGBROKER_TOKEN

        :param endpoint: endpoint инсталляции.
            Если не передан, будет использован перенаправляющий записи в случае отказа дц
            logbroker.yandex.net

        :param timeout: Таймаут на операции.
            Если не передан, будет взято значение из одноимённого атрибута класса.

        """
        self._api = None  # type: Optional[pqlib.PQStreamingAPI]
        self.timeout = timeout or self.timeout
        self.endpoint = endpoint or 'logbroker.yandex.net'

        token = token or environ.get('LOGBROKER_TOKEN', '')

        self.credentials = auth.OAuthTokenCredentialsProvider(token.encode())

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.quit()

    def quit(self):
        api = self._api
        api and api.stop()

    def _init(self):
        api = pqlib.PQStreamingAPI(self.endpoint, 2135)
        api_start_future = api.start()
        api_start_future.result(timeout=self.timeout)

        self._api = api

        return api

    @property
    def api(self):
        api = self._api

        if not api:
            api = self._init()

        return api

    def get_producer(self, source, topic, json_encoder=None, retrying=True):
        # type: (str, str, Optional[Type[json.JSONEncoder]], bool)  -> LogbrokerProducer
        """Возвращает писчего для определённого раздела.

        :param source: Идентификатор, описывающий источник. Произвольная строка.

        :param topic: Раздел, куда требуется производить запись.

        :param json_encoder: Кодировщик json, используемый при записи данных в виде словарей.

        :param retrying: тип писателя: обычный или retrying.

        """
        return LogbrokerProducer(client=self, source=source, topic=topic, json_encoder=json_encoder, retrying=retrying)


class LogbrokerProducer(object):
    """Писчий в логброкер.

    with LogbrokerClient() as logbroker:

        with logbroker.get_producer(source='me', topic='some/there/here') as producer:
            producer.write({'some': 'value'})
            producer.write('plaintext')

    """
    timeout = 10  # type: int

    def __init__(
        self,
        client,
        source,
        topic,
        timeout=None,
        json_encoder=None,
        retrying=True
    ):
        # type: (LogbrokerClient, str,str, int, Optional[Type[json.JSONEncoder]], bool) -> None
        """

        :param client: Экземпляр клиента.

        :param source: Наименование источника, от имени которого будет производится запись.

        :param topic: Наименование раздела, в который будет производиться запись.
            Может включать имена директорий.

        :param timeout:  Таймаут на операции.
            Если не передан, будет взято значение из одноимённого атрибута класса.

        :param json_encoder: Кодировщик json, используемый при записи данных в виде словарей.

        :param retrying: тип писателя: обычный или retrying.

        """
        self.timeout = timeout or self.timeout
        self.client = client
        self.seq_num = 0
        self.topic = topic
        self.source = source
        self.json_encoder = json_encoder
        self._me = None
        self.retrying = retrying

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def close(self):
        me = self._me
        if me:
            me.stop()
            self._me = None

    def init(self):
        """Инициализирует писателя.
        И одновременно получает последний использованный в последовательности идентификатор (.seq_num).

        В случае необходимости будет автоматически вызван в методе .write().
        Вызывать его вручную имеет смысл, когда требуется получить .seq_num до записи.

        """
        client = self.client

        configurator = pqlib.ProducerConfigurator(self.topic.encode(), self.source.encode())

        if self.retrying:
            producer = client.api.create_retrying_producer(configurator, credentials_provider=client.credentials)
        else:
            producer = client.api.create_producer(configurator, credentials_provider=client.credentials)

        start_future = producer.start()
        result = start_future.result(timeout=self.timeout)

        if isinstance(result, errors.SessionFailureResult):  # pragma: nocover
            raise LogbrokerException(result.description.error.description, result.description.error.code)

        if not result.HasField('init'):  # pragma: nocover
            raise LogbrokerException('Unable to initialize Logbroker producer: {result}'.format(result=result))

        self.seq_num = result.init.max_seq_no
        self._me = producer

        return producer

    def write(self, message, seq_num=None):
        # type: (Union[str, dict, bytes], int) -> int
        """Производит запись данных.

        Внимание! Измененять seq_num вручную в общем случае не нужно, это может привести к потере сообщений!

        :param message:

        :param seq_num: номер сообщения, по умолчанию инкрементируется max_seq_num.

        """
        producer = self._me

        if not producer:
            producer = self.init()

        seq_num = seq_num if seq_num else self.seq_num + 1
        self.seq_num = seq_num

        if isinstance(message, dict):
            message = json.dumps(message, cls=self.json_encoder)

        if isinstance(message, str):
            message = message.encode()

        try:
            response = producer.write(seq_num, message)

        except errors.ActorTerminatedException:
            self.close()
            return self.write(message=message, seq_num=seq_num)

        result = response.result(timeout=self.timeout)

        if not result.HasField('ack'):  # pragma: nocover
            raise LogbrokerException('Unable to write Logbroker message: {result}'.format(result=result))

        return seq_num
