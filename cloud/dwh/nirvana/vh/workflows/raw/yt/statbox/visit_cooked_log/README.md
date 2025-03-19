#### Visit Cooked Log

Подготовленный лог с данными счетчиков яндекса. Оригинальный лог выгруженный сервисом "Яндекс.Метрика" находиться [здесь](https://yt.yandex-team.ru/hahn/navigation?path=//logs/hit-log)

Так как уже данные лежат в yt, то просто настроена ссылка:

```bash
# preprod
yt link --proxy hahn --target-path //statbox/cooked_logs/visit-cooked-log/v1/1d --link-path //home/cloud-dwh/data/preprod/raw/statbox/visit_cooked_log

# prod
yt link --proxy hahn --target-path //statbox/cooked_logs/visit-cooked-log/v1/1d --link-path //home/cloud-dwh/data/prod/raw/statbox/visit_cooked_log
```

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/statbox/visit_cooked_logs)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/statbox/visit_cooked_logs)

##### Схема

Прод и препрод не отличаются. Детальная схема
находится [тут](https://wiki.yandex-team.ru/jandexmetrika/data/metrikatables/visit-v2-log/)

