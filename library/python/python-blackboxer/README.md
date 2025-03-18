[![Build Status](https://drone.yandex-team.ru/api/badges/common-python/python-blackboxer/status.svg)](https://drone.yandex-team.ru/common-python/python-blackboxer)

## Что это

Это замена для [python-blackbox](https://github.yandex-team.ru/common-python/python-blackbox).

Библиотека форкнута с [HaaS/python-blackboxer](https://github.yandex-team.ru/HaaS/python-blackboxer/)

В версии 2.0 проведен масштабный рефакторинг.

Особенности:

  * человеческий API
  * поддерживает python >= 2.7, python >= 3.2
  * использует requests, держит соединение


### Статус проекта

Бета.

Некоторый функционал билиотеки используется в продакшене в проектах:
 * https://github.yandex-team.ru/product-security/karma-api (нагрузка невысокая)
 * https://github.yandex-team.ru/sendr/sendr (нагрузка невысокая)


## Как использовать

### Инициализация

Обычная инициализация:

    from blackboxer import Blackbox
    blackbox = Blackbox(url='http://pass-test.yandex.ru/blackbox/')

### Примеры

Запрос к userinfo:

    blackbox.userinfo(login='test.tes2017', userip='127.0.0.1')
    {'users': [{'have_hint': True,
            'have_password': True,
            'id': '4006929137',
            'karma': {'value': 0},
            'karma_status': {'value': 0},
            'login': 'test.tes2017',
            'uid': {'hosted': False, 'lite': False, 'value': '4006929137'}}]}


### Ошибки

Если с блэкбоксом не удалось связаться выбрасывается исключение `TransportError`.

Если в ответе блэкбокса есть указание на [ошибку](https://doc.yandex-team.ru/blackbox/concepts/blackboxErrors.xml),
то выбрасывается `ResponseError`.

Если HTTP код ответа отличается от 200, выбрасывается `HTTPError`


### Таймаут и ретрай

Задать таймаут и ретрай можно следующим образом:

```
client = Blackbox(url='http://pass-test.yandex.ru/blackbox/', timeout=2, retry=1)
```

Ретрай расчитывается с помощью [exponential backoff](https://en.wikipedia.org/wiki/Exponential_backoff).

По умолчанию `backpff_factor = 0.3`, а количество попыток равно 3.
Это значит, что в общей сложности будет сделано 3 запроса с интервалами 0, 0.6 и 1.2 секунды

По умолчанию таймаут на выполнение запроса составляет 1 сек
С включеным `retry` таймауты ведут себя следующим образом:

```
# backoff_factor = 0.3, timeout = 1, retry = 3
# retry = 4.8 = 1 + 0 + 1 + 0.6 + 1 + 1.2
```

мы наблюдаем 3 запроса с таймаутом в 1 секунду и интервалами между запросами 0, 0.6, 1.2 секунды

### Окружение

Всевозможные урлы ЧЯ находятся в модуле environment.

Автоматически выбранный при помощи библиотеки `yenv` урл находится в переменной URL


    from blackboxer import URL, url_from_yenv

    # эти два клиента получили одинаковый урл
    auto_client = Blackbox(url=URL)
    auto_client2 = Blackbox(url=url_from_yenv())

    prod_client = Blackbox(url=url_from_yenv('intranet', 'production'))
 

## Разработка

### Запуск тестов

Тесты можно запускать в двух вариантах:
1. Используя mock сервера blackbox, подойдет для разработки, используется по умолчанию
```
py.test tests
```

2. Используя реальный сервер blackbox. Такой запуск необходим, чтобы убедится что интерфейс blackbox не изменился
```
py.test -p no:http_mock tests
```

Для создания mock сервера используется [HTTPretty](https://github.com/gabrielfalcao/HTTPretty).
Ответы mock сервера находятся в каталоге `tests/fixtures`. Файлы имеют одноименное название с тестируемой функцией.
## Другие реализации

Примеры проектов Яндексе с кастомной реализацией интерфеса с блэкбоксом: 

 * [Модуль в Паспорте](https://github.yandex-team.ru/passport/passport-core/tree/master/passport/builders/blackbox)
 * [Модуль в Диске](https://github.yandex-team.ru/MPFS/MPFS/blob/master/common/lib/mpfs/core/services/passport_service.py)
 * [common-python/python-blackbox](https://github.yandex-team.ru/common-python/python-blackbox)