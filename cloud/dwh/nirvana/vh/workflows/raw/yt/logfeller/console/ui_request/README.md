#### UI console requests

Логи запросов UI консоли облака

Так как данные уже лежат в yt, то просто настроена ссылка:

```bash
# preprod
yt link --proxy hahn --target-path //home/logfeller/logs/dataui-preprod-nodejs-log/1d --link-path //home/cloud-dwh/data/preprod/raw/logfeller/console/ui_request

# prod
yt link --proxy hahn --target-path //home/logfeller/logs/dataui-prod-nodejs-log/1d --link-path //home/cloud-dwh/data/prod/raw/logfeller/console/ui_request
```

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/logfeller/console/ui_request)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/logfeller/console/ui_request)
