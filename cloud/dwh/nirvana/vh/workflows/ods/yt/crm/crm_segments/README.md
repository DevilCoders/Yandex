## crm segments
#crm #crm_segments

Содержит информацию о сегментах.


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                            | Источники                                                                                                                         |
|---------|----------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [crm_segments](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_segments) | [raw-crm_segments](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_segments) |
| PREPROD | [crm_segments](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_segments) | [raw-crm_segments](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_segments) |


### Структура

| Поле                     | Описание                                                                                                                         |
|--------------------------|----------------------------------------------------------------------------------------------------------------------------------|
| crm_account_id           | id [crm аккаунта](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_accounts), `FK`                 |
| acl_crm_team_set_id      | id team_set (команд может быть несколько)                                                                                        |
| assigned_user_id         | присвоенный id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) , `FK`       |
| created_by               | создано                                                                                                                          |
| date_from                | начало действия сегмента                                                                                                         |
| date_from_ts             | начало действия сегмента, utc                                                                                                    |
| date_from_dttm_local     | начало действия сегмента, local tz                                                                                               |
| date_to                  | конец действия сегмента                                                                                                          |
| date_to_ts               | конец действия сегмента, utc                                                                                                     |
| date_to_dttm_local       | конец действия сегмента, local tz                                                                                                |
| date_entered_ts          | дата время ввода, utc                                                                                                            |
| date_entered_dttm_local  | дата время ввода, local tz                                                                                                       |
| date_modified_ts         | дата время изменения, utc                                                                                                        |
| date_modified_dttm_local | дата время изменения, local tz                                                                                                   |
| default_segment          | сегмент по умолчанию                                                                                                             |
| deleted                  | был ли сегмент удален                                                                                                            |
| crm_segment_description  | описание сегмента                                                                                                                |
| crm_segment_id           | id сегмента , `PK`                                                                                                               |
| modified_user_id         | id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) внесшего изменение, `FK` |
| crm_segment_name         | название сегмента                                                                                                                |
| segment_enum             | значение сегмента (список)                                                                                                       |
| segment_manually         | выполнен ли override сегмента?                                                                                                   |
| crm_team_id              | id [primary_team](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_teams), `FK`                    |
| crm_team_set_id          | id team_set (команд мб несколько)                                                                                                |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
