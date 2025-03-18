# Async Clients

Набор ассинхронных клиентов к различным сервисам Яндекса

## Пример использования
```
from async_client.utils import get_client

client = get_client(
    name='webmaster',
    host='https://webmaster.example.host.yandex.ru',
    service_ticket='service_ticket'
)

result = await client.info(...)
assert result = {'some': 'response'}
```

Можно так же импортировать напрямую, `get_clients` лишь шорткат
```
from async_clients.clients.webmaster import Client as WebmasterClient

client = WebmasterClient(
    host='https://webmaster.example.host.yandex.ru',
    service_ticket='service_ticket'
)
...
```

Если в клиенте не реализован нужный вам метод всегда можно дергать запросы напрямую
```
result = await client._make_request(
    path='some/path',
    method='post',
    params={'get': 'params'},
    data={'request': 'body'},
    headers={'my': 'headers'},
    # тут заголовки не полностью заменят дефолтные клиента, а лишь дополнят их
)
```
## Параметры аутентификации
Клиент может требовать передачу дополнительных обязательных параметров при инициализации
в зависимости от `AUTH_TYPES` клиента, например для `OAuth` это `token`
Параметры для других типов можно посмотреть в [auth_types.py](https://a.yandex-team.ru/arc/trunk/arcadia/library/python/async_clients/auth_types.py)
Если вы хотите использовать тип, который клиент по умолчанию не поддерживает, но который поддержан в
библиотеке - передайте нужно значение в `auth_type` при инициализации клиента, например:
```
client = WebmasterClient(
    host='https://webmaster.example.host.yandex.ru',
    auth_type='oauth'
    token='my_token'
)
```

## Написание нового клиента
Нужно унаследовать свой клиент от `async_clients.clients.base.BaseClient`
и назвать его `Client` (иначе через `get_client` его будет не получить)

Далее по необходимости переопределить:
`RESPONSE_TYPE` - тип ответа, по умолчанию `'json'`, где значение - одно из (`json, text, read`) [документация aiohttp](https://docs.aiohttp.org/en/stable/client_quickstart.html#response-content-and-status-code)
`AUTH_TYPES` - поддерживаемые севрисом типы аутентификации, все доступные смотри в [auth_types.py](https://a.yandex-team.ru/arc/trunk/arcadia/library/python/async_clients/auth_types.py)
по умолчанию - `(TVM2, OAuth)`
`async def parse_response` - для кастомной обработки результатам запроса (например, чтобы кидать ошибки
на `2хх` коды, если в теле есть `errors`)

## Запуск тестов
 `ya make -ttt tests`
