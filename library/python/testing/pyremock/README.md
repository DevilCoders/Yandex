# Описание
**pyremock** - библиотека для создания моков HTTP-сервисов на python.

Основное предназначение - тестирование взаимодействия по HTTP с внешними сервисами.

За образец взят аналогичный java-фреймворк [wiremock](http://wiremock.org).

# Быстрый старт

```python
import requests
from hamcrest import is_
from yatest.common import network
from library.python.testing.pyremock.lib.pyremock import mocked_http_server, MatchRequest, MockResponse

def test_quickstart():
    with network.PortManager() as pm:
        port = pm.get_port(9999)
        with mocked_http_server(port) as mock:
            request = MatchRequest(path=is_("/users"))
            response = MockResponse(status=200, body="alice,bob,charlie")
            mock.expect(request, response)
            actual = requests.get("http://localhost:%d/users" % port, timeout=1)
            assert actual.status_code == requests.codes.ok
            assert actual.text == "alice,bob,charlie"
```

# Пример использования

У нас есть программа *my_program*, выполняющая запросы по HTTP в сервис *ext_service*.
К примеру, она запрашивает информацию о пользователе запросом `GET /user_info?id=12345`.
Нам надо написать тесты на *my_program* для разных кейсов. Например, кейс когда *ext_service* отвечает `500 Internal Server Error`.

* Поднимаем мок

```python
port = 9999
mock = MockHttpServer(port)
mock.start()
```

* Указываем, что на нужный нам запрос мок отвечает ошибкой

```python
request = matchRequest(method=is_(HttpMethod.GET),
                       path=starts_with("/user_info"))
response = MockResponse(status=500, body='{"status": "server error"}')
mock.expect(request, response)
```

* Запускаем *my_program* с указанием адреса мока и проверяем результат

```python
bin = yatest.common.binary_path("my_program")
cmd = [bin, "--ext-service", "http://localhost:%d" % port]
yatest.common.execute(cmd)
# проверяем результат через assert
```

# Возможности

* Поиск подходящих запросов по URL-path, аргументам, заголовкам, POST-телу
* Конструирование ответа с нужным кодом, заголовками и телом
* Имитация задержки при ответе
* Повтор ответа заданное количество раз

Для матчинга запросов используется [pyhamcrest](http://pyhamcrest.readthedocs.io/)
Примеры можно найти в тестах в папке [tests](tests)
