bclclient
=========

Описание
--------

*Python-клиент для API сервиса BCL [балалайка]*

* `Сервис BCL <https://abc.yandex-team.ru/services/balalayka>`_
* `Описание API <https://wiki.yandex-team.ru/BALALAYKA/api/>`_


Локальная установка
-------------------

.. code-block:: bash

    pip install -e .


Модуль для Python
-----------------

Расширенное описание интерфейсов доступно в коде.

*Ниже приведены лишь некоторые примеры использования.*


Базовые действия
~~~~~~~~~~~~~~~~

.. code-block:: python

    from bclclient import Bcl, HOST_TEST

    # Используем тестовый стенд, авторизуемся с указанными реквизитами клиента.
    bcl = Bcl(auth=(TVM_CLIENT_ID, TVM_CLIENT_SECRET), host=HOST_TEST)

    # Получаем данные о внешних системах, поддерживаемых BCL.
    bcl.reference.get_associates()

    # Получаем информацию об указанных счетах.
    bcl.reference.get_accounts(['111', '222'])


Платежи
~~~~~~~

.. code-block:: python

    # Регистрируем платеж, указывая минимальный набор реквизитов.
    Payment = bcl.payments.cls_payment  # Псевдоним класса платежей для удобства.
    payments = [Payment(id='xxx-123', amount='10.5', acc_from='111', acc_to='222')]
    registered, failed = bcl_test.payments.register(*payments)

    # Аннулируем платёж.
    bcl.payments.cancel(ids=[payments[0]])

    # Отзываем патёж.
    bcl.payments.revoke(ids=[payments[0]])

    # Получаем данные платежей.
    payments = bcl.payments.get(ids=[payments[0], 'yyy-456'], upd_since=datetime.now())
    payment_id = payments[0].id
    payment_props = payments[0].props
    payment_status = payment_props['status']


Выписки
~~~~~~~

.. code-block:: python

    statements = bcl_test.statements.get(
        accounts=['111', '222'],
        on_date='2020-03-23',
        intraday=False
    )
    credit = statements[0].turnover_ct
    payment = statements[0].payments[0]


Проброс запросов
~~~~~~~~~~~~~~~~

.. code-block:: python

    paypal = bcl.proxy.paypal
    data = paypal.get_token(auth_code='1020')
    data = paypal.get_userinfo(token='dummytoken')

    data = bcl.proxy.pingpong.get_seller_status(seller_id='10')

    data = bcl.proxy.payoneer.get_payee_status(program_id='prog1', payee_id='payee1')
