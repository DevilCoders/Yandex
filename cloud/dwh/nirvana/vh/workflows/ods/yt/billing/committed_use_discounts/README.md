## Committed use discount
#billing #committed_use_discount

Вычитывает последний (актуальный) снапшот таблицы скидок за резерв (`committed_use_discounts`)


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                                                        | Источники                                                                                                                                                                        |
|---------|--------------------------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [committed_use_discount](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/committed_use_discounts)    | [raw-committed_use_discount](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/billing/hardware/default/billing/meta/committed_use_discounts)    |
| PREPROD | [committed_use_discount](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/committed_use_discounts) | [raw-committed_use_discount](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/billing/hardware/default/billing/meta/committed_use_discounts) |


### Структура

| Поле                      | Описание                                                       |
|---------------------------|----------------------------------------------------------------|
| billing_account_id        | идентификатор [платежного аккаунта](../billing_accounts), `FK` |
| end_ts                    | дата и время окончания применения, utc                         |
| end_dttm_local            | дата и время окончания применения, local tz                    |
| committed_use_discount_id | идентификатор скидки за резерв, `PK`                           |
| compensated_sku_id        | идентификатор [продукта](../skus), `FK`                        |
| created_ts                | дата и время создания записия, utc                             |
| created_dttm_local        | дата и время создания записия, local tz                        |
| pricing_quantity          | купленное кол-во                                               |
| pricing_unit              | единица покупки                                                |
| start_ts                  | дата и время начала применения, utc                            |
| start_dttm_local          | дата и время начала применения, local tz                       |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
