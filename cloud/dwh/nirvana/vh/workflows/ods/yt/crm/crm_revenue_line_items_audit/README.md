## crm revenue line items audit
#crm #crm_revenue_line_items_audit

Содержит аудит по изменению данных.

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                                                            | Источники                                                                                                                                                         |
|---------|------------------------------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [crm_revenue_line_items_audit](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_revenue_line_items_audit) | [raw-crm_revenue_line_items_audit](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_revenue_line_items_audit) |
| PREPROD | [crm_revenue_line_items_audit](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_revenue_line_items_audit) | [raw-crm_revenue_line_items_audit](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_revenue_line_items_audit) |


### Структура

| Поле                    | Описание                          |
|-------------------------|-----------------------------------|
| after_value_string      | значение после изменения (строка) |
| after_value_text        | значение после изменения (текст)  |
| before_value_string     | значение до изменения (строка)    |
| before_value_text       | значение до изменения (текст)     |
| created_by              | кем создано изменение             |
| data_type               | тип данных                        |
| date_created_ts         | дата создания, utc                |
| date_created_dttm_local | дата создания, local tz           |
| date_updated_ts         | дата изменения, utc               |
| date_updated_dttm_local | дата изменения, local tz          |
| event_id                | идентификатор события             |
| field_name              | поле                              |
| id                      | идентификатор изменения           |
| parent_id               | идентификатор измения родителя    |


### Загрузка
Статус загрузки: архив

Периодичность загрузки:
