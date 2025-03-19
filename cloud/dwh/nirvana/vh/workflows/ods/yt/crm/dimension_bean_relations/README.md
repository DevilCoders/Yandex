## Dimension bean_relations
#crm #dimension_bean_relations

Таблица описывает измерение объекта в CRM.


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                                                    | Источники                                                                                                                                                |
|---------|----------------------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [dimension_bean_relations](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/dimension_bean_relations) | [raw-dimension_bean_relations](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_dimensions_bean_rel) |
| PREPROD | [dimension_bean_relations](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/dimension_bean_relations) | [raw-dimension_bean_relations](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_dimensions_bean_rel) |


### Структура

| Поле                       | Описание                                                                                                              |
|----------------------------|-----------------------------------------------------------------------------------------------------------------------|
| crm_bean_id                | id объекта в CRM, с которым связано измерение                                                                         |
| crm_bean_module            | тип сущности, с которой связан dimension                                                                              |
| date_modified_ts           | дата время модификации, utc                                                                                           |
| date_modified_dttm_local   | дата время модификацииа, local tz                                                                                     |
| deleted                    | была ли связь удалена                                                                                                 |
| crm_dimension_id           | id [измерения в CRM](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_dimensions), `FK` |
| dimension_bean_relation_id | relation_id сущности, с которой связан dimension                                                                      |
| order_num                  | порядковый номер                                                                                                      |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
