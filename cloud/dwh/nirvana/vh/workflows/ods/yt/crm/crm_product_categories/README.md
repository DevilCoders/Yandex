## crm product categories
#crm #crm_product_categories

Содержит информацию о категориях продуктов.


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                                                | Источники                                                                                                                                             |
|---------|------------------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [crm_product_categories](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_product_categories) | [raw-crm_product_categories](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_product_categories) |
| PREPROD | [crm_product_categories](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_product_categories) | [raw-crm_product_categories](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_product_categories) |


##### Структура

| Поле                         | Описание                                                                                                                         |
|------------------------------|----------------------------------------------------------------------------------------------------------------------------------|
| assigned_user_id             | присвоенный id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) , `FK`       |
| created_by                   | создано                                                                                                                          |
| date_entered_ts              | дата время ввода, utc                                                                                                            |
| date_entered_dttm_local      | дата время ввода, local tz                                                                                                       |
| date_modified_ts             | дата время модификации, utc                                                                                                      |
| date_modified_dttm_local     | дата время модификацииа, local tz                                                                                                |
| deleted                      | удалена ли строка                                                                                                                |
| product_category_description | описание категории продукта                                                                                                      |
| product_category_id          | id категории продукта                                                                                                            |
| list_order                   | очередность вывода                                                                                                               |
| modified_user_id             | id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) внесшего изменение, `FK` |
| product_category_name        | название категории продукта                                                                                                      |
| parent_id                    | id родителя                                                                                                                      |
| service_long_name            | название сервиса (длинное)                                                                                                       |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
