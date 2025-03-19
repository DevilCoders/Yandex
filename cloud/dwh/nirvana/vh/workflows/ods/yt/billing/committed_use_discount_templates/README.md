## Committed use discount templates
#billing #committed_use_discount_templates

Вычитывает последний (актуальный) снапшот таблицы шаблонов скидок за резерв (`committed_use_discount_templates`)


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                                                                           | Источники                                                                                                                                                                                           |
|---------|---------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [committed_use_discount_templates](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/committed_use_discount_templates)    | [raw-committed_use_discount_templates](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/billing/hardware/default/billing/meta/committed_use_discount_templates)    |
| PREPROD | [committed_use_discount_templates](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/committed_use_discount_templates) | [raw-committed_use_discount_templates](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/billing/hardware/default/billing/meta/committed_use_discount_templates) |


### Структура

| Поле                               | Описание                                                    |
|------------------------------------|-------------------------------------------------------------|
| committed_use_discount_template_id | идентификатор шаблона скидки за резерв, `PK`                |
| compensated_sku_id                 | идентификатор [продукта](../skus), `FK`                     |
| created_ts                         | дата и время создания, utc                                  |
| created_dttm_local                 | дата и время создания, local tz                             |
| purchase_unit                      | единица измерения                                           |
| tiered_billed_schema_schema        | многоуровневая схема выставления счетов, схема              |
| tiered_billed_schema_timespan      | многоуровневая схема выставления счетов, временной интервал |
| constraint_purchase_quantity_min   | ограничения количества покупок, min                         |
| constraint_purchase_quantity_max   | ограничения количества покупок, max                         |
| constraint_purchase_quantity_step  | ограничения количества покупок, шаг                         |
| constraint_timespan_quantity_min   | ограничения количества временных интервалов, min            |
| constraint_timespan_quantity_max   | ограничения количества временных интервалов, max            |
| constraint_timespan_quantity_step  | ограничения количества временных интервалов, шаг            |
| is_private                         | флаг приватности                                            |
| order                              | очередность вывода                                          |
| updated_ts                         | дата и время изменений, utc                                 |
| updated_dttm_local                 | дата и время изменений, local tz                            |

### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
