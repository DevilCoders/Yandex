refsclient
==========
https://a.yandex-team.ru/arc/trunk/arcadia/library/python/refsclient


Описание
--------

*Python-клиент для сервиса Справочная [refs]*

Справочная (refs) является сервисом-посредником, получающий и складирующий информацию извне
для последующего предоставления потребителям внутри компании по средствам API.

`Сервис Справочная <https://abc.yandex-team.ru/services/refs>`_

Данный клиент может использоваться и как консольный клиент и как модуль для Питона.


Установка
---------

.. code-block:: bash

    pip install refsclient -i https://pypi.yandex-team.ru/simple/

    ; Либо, чтобы поставить с CLI:
    pip install refsclient[cli] -i https://pypi.yandex-team.ru/simple/


Консольное приложение
---------------------

``refsclient`` после установки делает доступной одноимённую консольную команду, при помощи которой
можно производить базовые действия.

.. code-block:: bash

    ; Вызов справки по консольному приложению с полным списком поддерживаемых команд.
    refsclient --help

    ; Доступные ресурсы для справочника swift на тестовом стенде (сокращённо `t`).
    refsclient --host t generic_get_resources swift

    ; Доступные типы для справочника swift на боевом стенде.
    refsclient generic_get_types swift

    ; Описания полей, доступных для типа Event
    refsclient generic_get_type_fields swift Event

    ; То, что и выше, но локализованное на русский (ru)
    refsclient --lang ru generic_get_type_fields swift Event

    ; Запрос информации об указанных BIC.
    refsclient swift_bic_info YNDMRUM1 SECTAEA1710 --fields active,eventDate,instName

    ; Запрос информации об указанных BIC.
    refsclient cbrf_banks_info 044525728 046311904 --fields nameFull,zip

    ; Запрос списка актуальных банков.
    refsclient cbrf_banks_listing --fields nameFull,zip


Модуль для Python
-----------------

Расширенное описание интерфейсов доступно в коде. Ниже приведены примеры использования.


**Базовые действия**

.. code-block:: python

    from refsclient import Refs, HOST_TEST

    # Используем тестовый стенд и просим локализовать выдачу на русский (ru).
    refs = Refs(host=HOST_TEST, lang='ru')

    # Доступные ресурсы для справочника. На примере swift.
    swift_resources = refs.generic_get_resources('swift')
    # Типы данных.
    swift_types = refs.generic_get_types('swift')
    # Поля для типа Event.
    swift_event_fields = refs.generic_get_type_fields('swift', 'Event')


**Справочник SWIFT**

.. code-block:: python

    swift = refs.get_ref_swift()

    swift.get_resources()  # Ресурсы справочника.
    swift.get_types()  # Типы данных
    swift.get_type_fields('Event')  # Поля для типа Event

    # Запрос информации об указанных BIC.
    bics = swift.get_bics_info(['YNDMRUM1', 'SECTAEA1710'])

    # Запрос нерабочих дней по РФ в пределах указанных дат.
    holidays = swift.get_holidays_info(
        date_since='2019-01-01',
        date_to='2019-01-20',
        countries=['RU'],
    )


**Справочник ЦБ РФ**

.. code-block:: python

    cbrf = refs.get_ref_cbrf()

    cbrf.get_resources()  # Ресурсы справочника.
    cbrf.get_types()  # Типы данных
    cbrf.get_type_fields('Bank')  # Поля для типа Bank

    # Запрос информации по банкам.
    banks = cbrf.get_banks_info(['044525728', '046311904'])

    # Запрос списка банков, создается итератор
    for bank in cbrf.banks_listing():
        print(bank)



**Справочник Валют**

.. code-block:: python

    currency = get_refs().get_ref_currency()

    currency.get_resources()  # Ресурсы справочника.
    currency.get_types()  # Типы данных
    currency.get_type_fields('Rate')  # Поля для типа Rate

    # Известные валюты.
    currencies = currency.get_listing()

    # Известные поставщики курсов валют.
    sources = currency.get_sources()

    # Курсы валют.
    rates = currency.get_rates_info()


**Справочник ФИАС**

.. code-block:: python

    fias = refs.get_ref_fias()

    fias.get_resources() # Ресурсы справочника.
    fias.get_types() # Типы данных
    fias.get_type_fields('AddrObj')  # Поля для типа AddrObj

    # Уровни адресных объектов, а также описание
    levels = fias.get_levels()

    # Информация по адресным объектам (области, улицы, города и т.д.).
    addresses = fias.get_addr_info(
        base_ids=['2ce195fb-fc4a-4a3a-9492-8be5da769cd6'],
        parent_ids=['8dea00e3-9aab-4d8e-887c-ef2aaa546456'],
        with_archived=True,
        levels=[7],
        name='Красно'
    )

    # Информация по домам и строениям.
    houses = fias.get_house_info(
        base_ids=['f58acc35-9f87-40d2-a8f6-014cec76fc42'],
        parent_ids=['2ce195fb-fc4a-4a3a-9492-8be5da769cd6'],
        with_archived=True,
        num='35'
    )

    # Информация по помещениям и квартирам.
    rooms = fias.get_room_info(
        base_ids=['0000000b-419e-41be-875b-583ff1655345'],
        parent_ids=['bdc0c198-0e2b-4298-b76c-ab6d8b57255c'],
        num='2',
        num_flat='59'
    )

    # Информация по участкам.
    steads = fias.get_stead_info(
        base_ids=['fba1f561-d91d-445f-8639-e6e2dc461514'],
        parent_ids=['7ba3a440-69fd-4b37-85e3-cdbdddd08229'],
        with_archived=True,
    )

    # Информация по документам.
    docs = fias.get_doc_info(
        base_ids=['ffffff6b-86bd-4f86-8f62-c302e8d401ad'],
    )


Запуск тестов
-------------

Тесты используют ``pytest``. Могут запускаться при помощи ``tox``:

.. code-block:: bash

    $ tox


Подход к разработке
-------------------

* На первом месте удобство и простота пользования интерфейсами;
* Публичные интерфейсы должны быть задокументированы;
* Код документируется на русском;
* Описание фиксаций (коммитов) производится на русском;
* Сущности должны носить осмысленные имена;
