## crm opportunity resources
#crm #crm_opportunity_resources

Содержит информацию о ресурсах.


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                                                      | Источники                                                                                                                                                   |
|---------|------------------------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [crm_opportunity_resources](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_opportunity_resources) | [raw-crm_opportunity_resources](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_opportunity_resources) |
| PREPROD | [crm_opportunity_resources](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_opportunity_resources) | [raw-crm_opportunity_resources](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_opportunity_resources) |


### Структура

| Поле                                 | Описание                                                                                                                         |
|--------------------------------------|----------------------------------------------------------------------------------------------------------------------------------|
| assigned_user_id                     | присвоенный id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) , `FK`       |
| created_by                           | строка создана                                                                                                                   |
| date_entered_ts                      | дата время ввода, utc                                                                                                            |
| date_entered_dttm_local              | дата время ввода, local tz                                                                                                       |
| date_modified_ts                     | дата время модификации, utc                                                                                                      |
| date_modified_dttm_local             | дата время модификацииа, local tz                                                                                                |
| deleted                              | удалена ли запись                                                                                                                |
| crm_opportunity_resource_description | описание ресурса                                                                                                                 |
| crm_opportunity_resource_id          | id, `PK`                                                                                                                         |
| modified_user_id                     | id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) внесшего изменение, `FK` |
| crm_opportunity_resource_name        | название ресурса                                                                                                                 |
| units                                | единица измерения ресурса                                                                                                        |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
