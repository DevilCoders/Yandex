#### MDB python api requests

Логи запросов в python api MDB сервисов. Это старое апи, на котором работают самые первые MDB сервисы: postgresql, mysql, mongodb, redis, clickhouse.

Так как данные уже лежат в yt, то просто настроена ссылка:

```bash
# preprod
yt link --proxy hahn --target-path //home/logfeller/logs/mdb-compute-preprod-iapi-log/1d --link-path //home/cloud-dwh/data/preprod/raw/logfeller/mdb/python_api_log

# prod
yt link --proxy hahn --target-path //home/logfeller/logs/mdb-compute-prod-iapi-log/1d --link-path //home/cloud-dwh/data/prod/raw/logfeller/mdb/python_api_log
```

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/logfeller/mdb/python_api_log)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/logfeller/mdb/python_api_log)
