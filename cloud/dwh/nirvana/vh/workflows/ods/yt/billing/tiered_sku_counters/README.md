## Tiered sku counters
#ods #billing #tiered_sku_counters

Cчетчик потребления для sku, где цена зависит от размера потребления (цена лесенкой).
Вычитывает последний (актуальный) снапшот таблицы (`tiered_sku_counters`).

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                                                 | Источники                                                                                                                                                                     |
|---------|-------------------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [tiered_sku_counters](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/tiered_sku_counters)    | [raw-tiered_sku_counters](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/billing/hardware/default/billing/counters/tiered_sku_counters)    |
| PREPROD | [tiered_sku_counters](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/tiered_sku_counters) | [raw-tiered_sku_counters](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/billing/hardware/default/billing/counters/tiered_sku_counters) |


### Структура
| Поле               | Описание                                                                                                                             |
|--------------------|--------------------------------------------------------------------------------------------------------------------------------------|
| billing_account_id | идентификатор [платежного аккаунта](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/billing/billing_accounts) |
| interval           | временной интервал                                                                                                                   |
| pricing_quantity   | оплачиваемое потребление                                                                                                             |
| shard_id           | id shard                                                                                                                             |
| sku_id             | идентификатор [sku](../skus), `FK`                                                                                                   |
| updated_ts         | дата и время изменений, utc                                                                                                          |
| updated_dttm_local | дата и время изменений, local tz                                                                                                     |


### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.
