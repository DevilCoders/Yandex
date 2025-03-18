# Python-клиента Секретницы

Документация Секретницы — [https://vault-api.passport.yandex.net/docs/](https://vault-api.passport.yandex.net/docs/)

Веб-интерфейс — [https://yav.yandex-team.ru](https://yav.yandex-team.ru)

Консольный клиент в Аркадии — `ya vault`

## Python API Quickguide

### Шаг 1. Подключаем библиотеку

В Аркадии:
```
PY2_PROGRAM()
...
PEERDIR(
	library/python/vault_client
)
...
```

Вне Аркадии через pip:
```
pip install yandex-passport-vault-client -i https://pypi.yandex-team.ru/simple
```

### Шаг 2. Создание секрета

Подключаем клиент в Аркадии
```
from library.python.vault_client.instances import Production as VaultClient
```

Или из пакета в PyPI:
```
from vault_client.instances import Production as VaultClient
```

Создадим секрет:
```
client = VaultClient(decode_files=True)
secret_uuid = client.create_secret('pepe-phone')
```

### Шаг 3. Добавление новой версии

```
version_uuid = client.create_secret_version(
    secret_uuid,
    {'phone': '+79161234567'},
)
```

### Шаг 4. Получение секрета

По айди секрета достанем самую свежую версию секрета:
```
head_version = client.get_version(secret_uuid)
```

Или конкретную версию секрета по айди версии:
```
version = client.get_version(version_uuid)
```

### Шаг 5. Выдача пишущего доступа на секрет коллеге

```
client.add_user_role_to_secret(
    secret_uuid,
    'owner',
    uid=1120000000038274,
)
```

### Шаг 6. Выдача читающего доступа на секрет своей группе

```
client.add_user_role_to_secret(
    secret_uuid,
    'reader',
    staff_id=31782,
)
```

## Авторизация по OAuth

По-умолчанию, клиент авторизуется в Секретнице по ssh-ключам из ssh-агента. Чтобы авторизоваться по OAuth-токену, получите токен для доступа к API Секретницы через команду `yav oauth` или [через браузер](https://oauth.yandex-team.ru/authorize?response_type=token&client_id=ce68fbebc76c4ffda974049083729982).

В питоновский клиент передайте заголовок в параметре authorization конструктора:
```
client = VaultClient(
    authorization='OAuth {}'.format('vault-api-token'),
    decode_files=True,
)
```
