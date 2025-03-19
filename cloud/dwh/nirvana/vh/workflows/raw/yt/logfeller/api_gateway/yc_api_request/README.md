#### Api gateway requests

Логи запросов в api gateway

Так как данные уже лежат в yt, то просто настроена ссылка:

```bash
# preprod
yt link --proxy hahn --target-path //home/logfeller/logs/yc-api-request-preprod-log/1d --link-path //home/cloud-dwh/data/preprod/raw/logfeller/api_gateway/yc_api_request

# prod
yt link --proxy hahn --target-path //home/logfeller/logs/yc-api-request-prod-log/1d --link-path //home/cloud-dwh/data/prod/raw/logfeller/api_gateway/yc_api_request
```

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/logfeller/api_gateway/yc_api_request)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/logfeller/api_gateway/yc_api_request)
