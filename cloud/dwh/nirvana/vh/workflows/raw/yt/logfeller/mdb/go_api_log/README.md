#### MDB go api requests

Логи запросов в go api MDB сервисов. Это новое апи, на котором работают относительно новые MDB сервисы: sql server, greenplum, elasticsearch, kafka.

Так как данные уже лежат в yt, то просто настроена ссылка:

```bash
# preprod
yt link --proxy hahn --target-path //home/logfeller/logs/mdb-compute-preprod-iapi-grpc-log/1d --link-path //home/cloud-dwh/data/preprod/raw/logfeller/mdb/go_api_log

# prod
yt link --proxy hahn --target-path //home/logfeller/logs/mdb-compute-prod-iapi-grpc-log/1d --link-path //home/cloud-dwh/data/prod/raw/logfeller/mdb/go_api_log
```

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/logfeller/mdb/go_api_log)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/logfeller/mdb/go_api_log)
