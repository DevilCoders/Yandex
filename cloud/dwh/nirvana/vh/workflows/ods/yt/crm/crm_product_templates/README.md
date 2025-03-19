## crm product templates
#crm #crm_product_templates

Содержит информацию о шаблонах продуктов.


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                                              | Источники                                                                                                                                        |
|---------|----------------------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [crm_product_templates](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_product_templates) | [raw-crm_product_templates](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_accounts_audit) |
| PREPROD | [crm_product_templates](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_product_templates) | [raw-crm_product_templates](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_accounts_audit) |


### Структура

| Поле                             | Описание                                                                                                                         |
|----------------------------------|----------------------------------------------------------------------------------------------------------------------------------|
| assigned_user_id                 | присвоенный id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) , `FK`       |
| crm_balance_id                   | id баланса                                                                                                                       |
| base_rate                        | rate валюты сделки                                                                                                               |
| crm_billing_id                   | id платежного аккаунта                                                                                                           |
| crm_category_id                  | id категории                                                                                                                     |
| cost_price                       | себестоимость                                                                                                                    |
| cost_usdollar                    | себестоимость в долларах                                                                                                         |
| created_by                       | созданный                                                                                                                        |
| crm_currency_id                  | id валюты                                                                                                                        |
| date_available_ts                | дата время начала начала доступности, utc                                                                                        |
| date_available_dttm_local        | дата время начала начала доступности, local tz                                                                                   |
| date_cost_price_ts               | дата время, utc                                                                                                                  |
| date_cost_price_dttm_local       | дата время, local tz                                                                                                             |
| date_entered_ts                  | дата время ввода, utc                                                                                                            |
| date_entered_dttm_local          | дата время ввода, local tz                                                                                                       |
| date_modified_ts                 | дата время изменения, utc                                                                                                        |
| date_modified_dttm_local         | дата время изменения, local tz                                                                                                   |
| date_modified_from_yt_ts         | дата время изменения в yt, utc                                                                                                   |
| date_modified_from_yt_dttm_local | дата время изменения в yt, local tz                                                                                              |
| deleted                          | удален ли шаблон                                                                                                                 |
| crm_product_template_description | описание шаблона продукта                                                                                                        |
| discount_price                   | скидка                                                                                                                           |
| discount_usdollar                | скидка в базовой валюте                                                                                                          |
| ext_category_id                  | внешний код категории                                                                                                            |
| crm_product_template_id          | id шаблона продукта                                                                                                              |
| list_order                       | очередность продукта в list view                                                                                                 |
| list_price                       | list price                                                                                                                       |
| list_usdollar                    | list price в базовой валюте                                                                                                      |
| modified_user_id                 | id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) внесшего изменение, `FK` |
| crm_product_template_name        | название шаблона продукта                                                                                                        |
| pricing_factor                   | фактор ценообразования                                                                                                           |
| pricing_formula                  | формула для расчета цены                                                                                                         |
| product_name_en                  | название продукта (на английском)                                                                                                |
| crm_sku_id                       | id продукта                                                                                                                      |
| sku_name                         | название продукта                                                                                                                |
| status                           | статус                                                                                                                           |
| tax_class                        | тип налообложения по продукту                                                                                                    |
| type_id                          | тип                                                                                                                              |
| units                            | единица измерения                                                                                                                |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
