## crm opportunity resource lines
#crm #crm_opportunity_resource_lines

Содержит информацию о ресурсах (строках), запрошенных в рамках сделки.


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                                                                | Источники                                                                                                                                                             |
|---------|----------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [crm_opportunity_resource_lines](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_opportunity_resource_lines) | [raw-crm_opportunity_resource_lines](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_opportunity_resource_lines) |
| PREPROD | [crm_opportunity_resource_lines](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_opportunity_resource_lines) | [raw-crm_opportunity_resource_lines](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_opportunity_resource_lines) |


### Структура

| Поле                                      | Описание                                                                                                                         |
|-------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------|
| acl_crm_team_set_id                       | id team_set (команд может быть несколько)                                                                                        |
| assigned_user_id                          | присвоенный id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) , `FK`       |
| created_by                                | строка создана                                                                                                                   |
| date_entered_ts                           | дата время ввода, utc                                                                                                            |
| date_entered_dttm_local                   | дата время ввода, local tz                                                                                                       |
| date_modified_ts                          | дата время модификации, utc                                                                                                      |
| date_modified_dttm_local                  | дата время модификацииа, local tz                                                                                                |
| deleted                                   | удалена ли запись                                                                                                                |
| crm_opportunity_resource_line_description | описание                                                                                                                         |
| crm_opportunity_resource_line_id          | id  , `PK`                                                                                                                       |
| modified_user_id                          | id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) внесшего изменение, `FK` |
| crm_opportunity_resource_line_name        | название ресурса (строки), запрошенного в рамках сделки                                                                          |
| crm_opportunity_id                        | id [сделки](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_opportunities), `FK`                  |
| crm_opportunity_resource_id               | id ресурса (строки), запрошенного в рамках сделки                                                                                |
| quantity                                  | количество ресурсов (в ед измерения)                                                                                             |
| crm_team_id                               | id [primary_team](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_teams), `FK`                    |
| crm_team_set_id                           | id team_set (команд мб несколько)                                                                                                |
| units                                     | единица измерения ресурса                                                                                                        |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
