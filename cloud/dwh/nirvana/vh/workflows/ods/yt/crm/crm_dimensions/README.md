## crm dimensions
#crm #crm_dimensions

Содержит информацию об измерениях.


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных

| Контур    | Расположение данных   | Источники |
| --------- | -------------------   | --------- |
| PROD      | [crm_dimensions](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_dimensions) | [raw-crm_dimensions](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_dimensions) |
| PREPROD   | [crm_dimensions](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_dimensions) | [raw-crm_dimensions](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_dimensions) |


### Структура

| Поле                      | Описание                                                                                                                         |
|---------------------------|----------------------------------------------------------------------------------------------------------------------------------|
| assigned_user_id          | присвоенный id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) , `FK`       |
| crm_category_id           | id категории                                                                                                                     |
| color                     | цвет                                                                                                                             |
| created_by                | создано                                                                                                                          |
| date_entered_ts           | дата время ввода, utc                                                                                                            |
| date_entered_dttm_local   | дата время ввода, local tz                                                                                                       |
| date_modified_ts          | дата время модификации, utc                                                                                                      |
| date_modified_dttm_local  | дата время модификацииа, local tz                                                                                                |
| deleted                   | было ли измерение удалено                                                                                                        |
| crm_dimension_description | описание измерения                                                                                                               |
| disable_use_alone         |                                                                                                                                  |
| crm_dimension_id          | id измерения, `PK`                                                                                                               |
| level                     | уровень                                                                                                                          |
| modified_user_id          | id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) внесшего изменение, `FK` |
| crm_dimension_name        | название измерения                                                                                                               |
| parent_id                 | id родителя                                                                                                                      |
| root_id                   | id родителя 0 уровня                                                                                                             |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
