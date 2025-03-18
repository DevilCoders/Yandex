errorboosterclient
==================


Описание
--------

*Клиент для отправки данных в ErrorBooster*

Предоставляет:

* инструменты для отправки данных в ErrorBooster через интерфейс ``sentry-sdk``.


Использование с **sentry-sdk**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Вводная: https://clubs.at.yandex-team.ru/python/3274

.. code-block:: python

    from errorboosterclient.sentry import ErrorBoosterTransport

    def send_me(event: dict):
        """Эта функция должна реализовывать отправку сообщений о событии в LogBroker."""

    # Инициализируем клиент как обычно, но передаём ErrorBoosterTransport.
    sentry_sdk.init(
        transport=ErrorBoosterTransport(
            project='myproject',  # Псевдоним проекта в бустере.
            sender=send_me,

            # Если в проекте пароли-явки могут быть в значениях переменных,
            # можно применить санацию и к этим значениям, проиграв в производительности.
            # О санации читайте ниже.
            sanitize_values=True,
        ),
        environment='development',
        release='0.0.1',

        # Если нужно подкрутить время, выделенное
        # на работу фоновой отправляющей нити.
        shutdown_timeout=20,

        # Для интеграции с Django:
        integrations=[DjangoIntegration()],
        send_default_pii=True,

        # По умолчанию клиент Sentry будет собирать и отсылать значения локальных
        # переменных из фреймов, затрагиваемых исключением (в бустер они придут в additional.vars).
        # Ввиду того, что переменные могут хранить чувствительные даннные, пароли, токены и пр.
        # можно отключить сбор таких данных вовсе при помощи означенной ниже опции.
        # Если не отключать, то наша реализация попытается скрыть под звёздочками
        # значения, если имена переменных содержат подстроку secret, password, apikey или подобные.
        # Если хочется расширить список слов, то можно наследоваться от EventConverter
        # и задать MASK_NAMES. После чего наследоваться от ErrorBoosterTransport перекрыть
        # converter_cls, указав свой класс.
        with_locals=False,

    )

    # Ручная отправка и проставление контекста производится обычным для сентри-сдк путём:
    with sentry_sdk.configure_scope() as scope:

        # Можно отправлять дополнительные данные о пользователе.
        scope.user = {'ip_address': '127.0.0.1', 'yandexuid': '1234567'}

        # А можно и дополнительные данные, поддерживаемые бустером.
        # Их перечень можно посмотреть в errorboosterclient.sentry.EventConverter
        scope.set_tag('source', 'backend1')

        try:
            import json
            json.loads('bogus')

        except:
            sentry_sdk.capture_exception()


Отправка из Deploy через Unified Agent
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Если ваше приложение находится в Deploy, то можно использовать HTTP интерфейс Unified Agent для отправки событий Sentry.

В Deploy нужно настроить:

1. ``Runtime Version`` >= 13
2. ``Logbroker Tools`` >= 28.01.2022 17:28 (2739742779)

При конфигурировании использовать UnifiedAgentSender.

.. code-block:: python

    import sentry_sdk
    from errorboosterclient.sentry import ErrorBoosterTransport
    from errorboosterclient.uagent import UnifiedAgentSender

    sentry_sdk.init(
        transport=ErrorBoosterTransport(
            project='myproject',
            sender=UnifiedAgentSender(),
        ),
        ...
    )


Отправка через Logbroker
~~~~~~~~~~~~~~~~~~~~~~~~

В комплекте библиотеки есть код, минимально необходимый для записи событий Sentry напрямую в Logbroker.

.. code-block:: python

    import sentry_sdk
    from errorboosterclient.logbroker import LogbrokerClient
    from errorboosterclient.sentry import ErrorBoosterTransport
    from sentry_sdk.integrations.django import DjangoIntegration

    ERR_LOGBROKER_PRODUCER = LogbrokerClient().get_producer(
        source='dev_err_events',
        topic='project/dev/trash',
    )

    sentry_sdk.init(
        transport=ErrorBoosterTransport(
            project='myproject',
            sender=ERR_LOGBROKER_PRODUCER.write
        ),
        ...
    )

Если у вас аркадийная сборка, не забудьте добавить в ``ya.make``:

.. code-block::

    PEERDIR(
        kikimr/public/sdk/python/persqueue
        ...
    )



Утилиты
~~~~~~~

Иногда может потребоваться отправить в бустер перехваченное нами же исключение
или эмулировать исключение, чтобы в бустер улетела некая информация.

Для этого можно использовать инструменты класса ``errorboosterclient.sentry.ExceptionCapturer``
(унаследовавшисть от оного в случае необходимости).

.. code-block:: python

    from errorboosterclient.sentry import ExceptionCapturer

    captured = ExceptionCapturer.raise_capture('send me to booster')


Удобное представление данных в ErrorBooster
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Для представления в ErrorBooster данных, которые отсылает ``errorboosterclient`` в более удобном виде
можно выбрать в разделе ``Items`` в раскрывающемся списке ``Formatter`` пункт ``sentry_python``.

Анонс: https://clubs.at.yandex-team.ru/python/3505



Установка
---------

PyPI:
~~~~~

.. code-block::

    $ pip install -i http://pypi.yandex-team.ru/simple/ --trusted-host pypi.yandex-team.ru errorboosterclient

    # Если используем встроенный errorboosterclient.logbroker.LogbrokerClient дописать ещё:
    # ydb-persqueue (раньше называлось kikimrclient)



Аркадия:
~~~~~~~~

.. code-block::

    PEERDIR(
        contrib/python/sentry-sdk
        contrib/python/sentry-sdk/sentry_sdk/integrations/django

        # Если используем встроенный errorboosterclient.logbroker.LogbrokerClient
        # kikimr/public/sdk/python/persqueue

        library/python/errorboosterclient
    )
