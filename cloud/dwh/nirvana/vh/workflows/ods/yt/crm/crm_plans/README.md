## crm plans
#crm #crm_plans

Содержит информацию о планах.


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                      | Источники                                                                                                                   |
|---------|----------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------|
| PROD    | [crm_plans](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_plans) | [raw-crm_plans](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_plans) |
| PREPROD | [crm_plans](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_plans) | [raw-crm_plans](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_plans) |


### Структура

| Поле                     | Описание                                                                                                                         |
|--------------------------|----------------------------------------------------------------------------------------------------------------------------------|
| crm_account_id           | id [аккаунта в CRM](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_accounts), `FK`               |
| acl_crm_team_set_id      | id team_set (команд может быть несколько)                                                                                        |
| assigned_user_id         | присвоенный id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) , `FK`       |
| base_rate                | rate валюты сделки                                                                                                               |
| created_by               | создано                                                                                                                          |
| crm_currency_id          | id валюты                                                                                                                        |
| date_entered_ts          | дата время ввода, utc                                                                                                            |
| date_entered_dttm_local  | дата время ввода, local tz                                                                                                       |
| date_modified_ts         | дата время модификации, utc                                                                                                      |
| date_modified_dttm_local | дата время модификацииа, local tz                                                                                                |
| deleted                  | были ли запись удалена                                                                                                           |
| crm_plan_description     | описание плана                                                                                                                   |
| crm_plan_id              | id плана , `PK`                                                                                                                  |
| is_target                | целевой?                                                                                                                         |
| modified_user_id         | id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) внесшего изменение, `FK` |
| crm_plan_name            | название плана                                                                                                                   |
| plan_type                | тип плана                                                                                                                        |
| segment                  | сегмент                                                                                                                          |
| selected_time_period     | id timeperiod                                                                                                                    |
| status                   | статус                                                                                                                           |
| crm_team_id              | id [primary_team](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_teams), `FK`                    |
| crm_team_set_id          | id team_set (команд мб несколько)                                                                                                |
| value_currency           | значение, если тип = валюта                                                                                                      |
| value_logical            | значение, если тип = логический                                                                                                  |
| value_numeric            | значение, если тип = число                                                                                                       |
| value_type               | тип значения                                                                                                                     |
| weight                   | вес плана                                                                                                                        |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
