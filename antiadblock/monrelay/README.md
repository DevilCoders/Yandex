## Сервис Антиадблока для сбора статустов партнерских мониторингов из разных источников

Локальный запуск (для дебага):
- Создать файл test-config-name с содержимым:
```json
{
    "configs_api_host": "{admin-stand-backend-url}",
    "monrelay_tvm_id": "2010820",
    "yt_token": "{your yt token}",
    "yql_token": "{your yql token}",
    "configs_api_tvm_id": "2000627",
    "key": "localrun",
    "monrelay_tvm_secret": "{monrelay tvm secret}"
}
```
- Собрать и запустить:
```bash
ya make
./monrelay -c test-config-name 
```

Как собирать и деплоить для прода описанов папке [./samogon](https://a.yandex-team.ru/arc/trunk/arcadia/antiadblock/monrelay/samogon)
