## crm product templates audit
#crm #crm_product_templates_audit

Содержит аудит по изменению данных [шаблонов продуктов](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_product_templates).


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                                                          | Источники                                                                                                                                                       |
|---------|----------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [crm_product_templates_audit](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_product_templates_audit) | [raw-crm_product_templates_audit](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_product_templates_audit) |
| PREPROD | [crm_product_templates_audit](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_product_templates_audit) | [raw-crm_product_templates_audit](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_product_templates_audit) |


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
Статус загрузки: активна

Периодичность загрузки: 1h.
