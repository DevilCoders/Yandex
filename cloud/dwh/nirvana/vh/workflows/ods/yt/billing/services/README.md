#### services:

Вычитывает последний (актуальный) снапшот таблицы `services`

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/services)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/services)
/ [PROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod_internal/ods/billing/services)
/ [PREPROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod_internal/ods/billing/services)

* `service_id`  - Уникальный идентификатор сервиса
* `name`        - Краткий код облачного сервиса
* `description` - Краткое описание сервиса
* `group`       - Группа сервисов (e.g. Cloud Native)
