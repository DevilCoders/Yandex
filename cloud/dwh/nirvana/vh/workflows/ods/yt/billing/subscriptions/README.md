## Subscriptions
#ods #billing #subscriptions

Вычитывает последний (актуальный) снапшот таблицы подписок (`subscriptions`)


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                                     | Источники                                                                                                                                                     |
|---------|-------------------------------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [subscriptions](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/subscriptions)    | [raw-subscriptions](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/billing/hardware/default/billing/meta/subscriptions)    |
| PREPROD | [subscriptions](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/subscriptions) | [raw-subscriptions](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/billing/hardware/default/billing/meta/subscriptions) |


### Структура

| Поле               | Описание                                    |
|--------------------|---------------------------------------------|
| subscription_id    | идентификатор подписки, `PK`                |
| owner_id           | идентификатор владельца                     |
| owner_type         | тип владельца                               |
| created_ts         | дата и время создания записи, utc           |
| created_dttm_local | дата и время создания записи, local tz      |
| end_ts             | дата и время окончания применения, utc      |
| end_dttm_local     | дата и время окончания применения, local tz |
| schema             | схема                                       |
| start_ts           | дата и время начала применения, utc         |
| start_dttm_local   | дата и время начала применения, local tz    |
| template_id        | идентификатор шаблона                       |
| template_type      | тип шаблона                                 |
| updated_ts         | дата и время изменений, utc                 |
| updated_dttm_local | дата и время изменений, local tz            |
| purchase_unit      | единица измерения (покупка)                 |
| usage_unit         | единица измерения (использование)           |
| purchase_quantity  | закупленное количество                      |
| labels             | название                                    |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
