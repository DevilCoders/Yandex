#### organizations

Так как уже данные лежат в yt, то просто настроена ссылка:

```bash
# preprod
yt link --proxy hahn --target-path //home/yandex-connect/prod/analytics/users --link-path //home/cloud-dwh/data/preprod/raw/yandex_connect/users

# prod
yt link --proxy hahn --target-path //home/yandex-connect/prod/analytics/users --link-path //home/cloud-dwh/data/prod/raw/yandex_connect/users
```

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/yandex-connect/prod/analytics/users)

##### Схема

Прод и препрод не отличаются. Схема находится [тут](https://wiki.yandex-team.ru/Connect/YandexConnectAnalytics/YaConnectLogs/)

