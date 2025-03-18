dssclient
=========
https://a.yandex-team.ru/arc/trunk/arcadia/library/python/dssclient


Описание
--------

*Python-клиент для КриптоПро DSS*

КриптоПро DSS (Digital Signature Service) - веб-сервис, позволяющий осуществлять цифровую подпись
различного рода документов.

**dssclient** является клиентом для указанного сервиса, позволяющим подписывать документы из кода
на Python, а также из командной строки.


Установка
---------

.. code-block:: bash

    pip install dssclient -i https://pypi.yandex-team.ru/simple/

    ; Либо, чтобы поставить со всеми зависимостями:
    pip install dssclient[all] -i https://pypi.yandex-team.ru/simple/


Консольное приложение
---------------------

``dsslicent`` после установки делает доступной одноимённую консольную команду, при помощи которой
можно производить базовые действия.

.. code-block:: bash

    ; Вызов справки по консольному приложению.
    dssclient --help

    ; Авторизуемся при помощи ключа --auth, например:
    ; --auth "username=BBB;password=CCC"
    ; Далее этот ключ опущен.

    ; Выводим данные обо всех доступных пользователю сертификатах.
    dssclient certificates list

    ; Выводим данные обо всех пользовательских запросах на выпуск сертификатов.
    dssclient certificate_requests list

    ; Подписываем файл tosign.txt в из текущей директории при помощи ГОСТ Р 34.10‑2001
    ; выводим в виде строки, кодированной в base64
    dssclient sign --base64 --file tosign.txt

    ; Подписываем строку "signthis". Выводим в виде байт в файл signed.txt.
    dssclient sign --string signthis > signed.txt

    ; Подписываем XML.
    dssclient sign --type xml --file tosign.xml

    ; Подписываем PDF и сохраняем подписанный файл как signed.pdf.
    dssclient sign --type pdf --file tosign.pdf > signed.pdf

    ; Подписываем документ MS Word.
    dssclient sign --type msoffice --file tosign.docx > signed.docx


Модуль для Python
-----------------

Расширенное описание интерфейсов доступно в коде. Ниже приведены примеры использования.


**Базовые действия**

.. code-block:: python

    from dssclient import Dss, HOST_TEST

    dss = Dss(dict(
        # Указываем реквизиты клиента OAuth.
        client_id='yandex-test-client',
        client_secret='Ez3&1?xKUq4_!7LtvT}5P=9s+jD08-oN',

        # Авторизуемся, как владелец ресурса (см. Resource Owner Grant),
        # указывая реквизиты, чтобы получить токен доступа автоматически.
        # Внимание: помимо Resource Owner Grant, DSS позволяет использовать
        # также Authorization Code, и Implicit Grant.
        username='BBB',
        password='CCC',

    ), host=HOST_TEST)  # Подключаемся к тестовому серверу.

    certificates = dss.certificates.get_all()

    # Пробегаемся по сертификатам, выводим основные данные.
    for certificate in certificates:
        print('%s: expires at %s' % (
            certificate.parsed.subject['common_name'],
            certificate.parsed.date_expires))

    # Если сертификат у пользователя один, то можно сделать его
    # сертификатом для подписи по умолчанию, дабы не указывать
    # каждый раз при подписывании.
    cert = certificates[0]
    cert.set_default()


**Регистрация сертификатов**

Генерируем сертификат прямо на DSS:

.. code-block:: python

    # Делаем запрос на сертификат. По умолчанию используется встроенный тестовый УЦ,
    # поэтому, если запрос завершился успешно, на его основе сразу будет создан сам сертификат.
    certificate_request = dss.certificates.requests.register(
        # Common Name будет достаточно для данного УЦ, передаём его строкой.
        subject='Some subject',
        # Указываем объектный идентификатор шаблона, по которому будет создан сертификат.
        # В данном случае это шаблон, созданный для нашего пользователя DSS.
        template='1.3.6.1.5.5.7.3.2',
    )


**Подписывание простой подписью ГОСТ Р 34.10‑2001**

.. code-block:: python

    # Подписываем строку и данные из указанного файла одним пакетом.
    signed_documents = dss.documents.sign(
        ['sign this string', dss.cls_file('/files/signthis.txt')],
        # Для подписи используем первый доступный сертификат.
        # Если нужно подписать сертификтаом по умолчанию (см. выше),
        # то сертификат можно не передавать.
        params=dss.signing_params.ghost3410(cert))

    for document in signed_documents:
        # Выводим результат подписи в base64.
        print('%s\n%s' % (document.name, document.signed_base64))
        print('==' * 30)


**Подписывание XML**

По умолчанию используется *Обёрнутая подпись* (вставляется внутрь корневого тега документа).

.. code-block:: python

    signed_documents = dss.documents.sign(
        dss.cls_file('/files/signthis.xml'),
        params=dss.signing_params.xml(cert))


**Подписывание документов MS Office**

Ожидаются документы Word или Excel.

.. code-block:: python

    signed_documents = dss.documents.sign(
        dss.cls_file('/files/signthis.docx'),
        params=dss.signing_params.msoffice(cert))


**Подписывание PDF**

По умолчанию используется формат *CMS (PKCS7)*.

.. code-block:: python

    signed_documents = dss.documents.sign(
        dss.cls_file('/files/signthis.pdf'),
        params=dss.signing_params.pdf(cert))



В PDF можно вставить печать с произвольным текстом, которая будет видна на странице:

.. code-block:: python

    signing_results = dss.documents.sign(
        dss.cls_file('/files/signthis.pdf'),
        params=dss.signing_params.pdf(cert, stamp=[
            # В начале строки поставим маркер формата (см. описание ниже).
            '!s:iu;f:arial!Документ подписан при помощи dssclient',
            'Сертификат: %s' % cert.serial,
            'Субъект: %s' % cert.subject,
        ]),
    )
    # И сохраним подписанный PDF с печатью в файл:
    signing_results[0].save_signed('/files/signed.pdf')


Используя ``stamp.set_background_image()`` для текстовых штампов, можно установить подложкой произвольное изображение.
Помимо этого, изображение можно использовать в качестве самой печати: ``stamp.set_foreground_image()``.


*Мини-язык форматирования для текстов печатей*

Строка должна начинаться с ! (воскл. знак), далее должны следовать пары маркер_форматирования:значение,
разделённые ; (точка с запятой), а заканчиваться инструкции форматирования должны на !.

Доступные маркеры и значения:

    * s - стиль шрифта; значения: i - курсив, b - полужирный, u - подчёркнутый, s - перечёркнутый);
    * S - размер шрифта, целое;
    * f - семейство шрифта; значение: arial, times;
    * c - цвет шрифта, строка; примеры значений: black, red, blue, green, gray и пр.
    * m - отступ текста, целое.



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
