## Asgi Yauth

Библиотека реализует `asgi` совместимую `middleware`
для авторизации, сейчас поддерживаются следующие бекэнды:

```
- tvm2
```

### Подключение
Для подключения нужно обернуть свое приложение `middleware` вот так:
```python3
from asgi_yauth import YauthMiddleware, config

application = YauthMiddleware(application, config=config)
```
где `application`, например, объект [Django application](https://docs.djangoproject.com/en/dev/howto/deployment/asgi/#applying-asgi-middleware)

Если вы используете производные от `starlette` (например `fastapi`)
то подключить можно так:
```python3
from asgi_yauth import YauthMiddleware, config

app = FastApi()

app.add_middleware(YauthMiddleware, config=config)
```

### Переопределние/определение настроек
Приложение позволяет переопределить большинство своих настроек, для
работы с настройками используется библиотека [pydantic](https://pydantic-docs.helpmanual.io/usage/settings/)

Для переопределения настроек достаточно передавать свои настройки через
переменные окружения, например:
```
export YAUTH_BACKENDS = ["import.path.to.my.backend", "tvm2"] - обрати внимание
встроенные в asgi_yauth бекэнды можно передавать без полного пути до них

export YAUTH_TVM2_CLIENT = 123

from asgi_yauth import config
config.backends -> [module_backend.Backend]  # да это будет не строка, а класс
config.tvm2_client -> 123
```
Бекэнды при авторизации вызываются по очереди - если какой-либо из них применим и
вернул пользователя - обход заканчивается.

Так же можно при желании унаследовать свой объект настроек и передавать его
при инициализации и добавлять новые настройки для своего бекэнда
```
from asgi_yauth import AsgiYauthConfig

class MyYauthConfig(AsgiYauthConfig):
    value_for_my_backend: str = 'smth'
```

Или же изменять настройки по умолчанию
```
from asgi_yauth import AsgiYauthConfig

class MyYauthConfig(AsgiYauthConfig):
    tvm2_allowed_clients = ['123', '222']
```

И затем передавать конфиг при инициализации
```
app.add_middleware(YauthMiddleware, config=MyYauthConfig())
```

### Использование
После авторизации объект `user` можно получить через
`scope['user']` или же, если исользуются производные `starlette` - через `request.user`

Пример с использованием `fastapi`:
```
from fastapi import Request

@app.get("some-path")
async def my_endpoint(request: Request):
    if not request.user.is_authenticated():
        raise PermissionDenied()

    do_smth()
    if request.user.auth_type == 'user_ticket':
        user_ticket = request.user.raw_user_ticket
        go_to_some_api_with_user_ticket(ticket=user_ticket)
```

### Использование в тестах
При необходимости в тестах можете заменять обычную `middleware` на `YauthTestMiddleware`
которая всегда успешно авторизует пользователя и возвращает класс пользователя
из `config.test_user_class` наполненый данными из `config.test_user_data`
В своих тестах можете соответствено переопределять эти параметры при необходимости

### TVM2
Для корректной работы TVM2 необходимо определиться с типом используемого клиента (`тред`/`демон`)

И задать следующие переменные окружения
`YAUTH_TVM2_SECRET` - секрет приложения (не нужно при использовании `daemon` версии)
`YAUTH_TVM2_CLIENT` - client_id своего приложения
`YAUTH_TVM2_ALLOWED_CLIENTS` - список клиентов которым разрешено приходить в приложение (пустой список по умолчанию)
`TVM2_BLACKBOX_CLIENT` - тип черного ящика для проверки пользовательских тикетов, варианты смотрите в
классе `BlackboxClient` (`продовый yateam` паспорт по умолчанию)

Более подробно про использование tvm2 читайте в [описании библиотеки](https://a.yandex-team.ru/arc/trunk/arcadia/library/python/tvm2)

### Реализация собственного бекэнда
Для реализации собственного бекэнда нужно унаследовать свой класс от `BaseBackend`,
назвать его `Backend` и реализовать методы `extract_params` и `authenticate`.
И прописать путь до модуля с классом в конфиге - `export YAUTH_BACKENDS = ["import.path.to.my.backend"]`

### Запуск тестов
`ya make --test-stdout -ttt tests`

### Выпуск новой версии
`ya tool releaser changelog`
