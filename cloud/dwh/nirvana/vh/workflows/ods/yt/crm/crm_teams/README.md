## crm teams
#crm #crm_teams

Содержит информацию о командах.


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Структура PII.](#структура_PII)
4. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                                                                                                                                           | Источники                                                                                                                   |
|---------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------|
| PROD    | [crm_teams](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_teams), [PII-crm_teams](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/crm/PII/crm_teams) | [raw-crm_teams](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_teams) |
| PREPROD | [crm_teams](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_teams), [PII-crm_teams](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/crm/PII/crm_teams) | [raw-crm_teams](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_teams) |


### Структура

| Поле                      | Описание                                                                                                                         |
|---------------------------|----------------------------------------------------------------------------------------------------------------------------------|
| associated_user_id        | id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users), `FK`                    |
| created_by                | создано                                                                                                                          |
| date_entered_ts           | дата время ввода, utc                                                                                                            |
| date_entered_dttm_local   | дата время ввода, local tz                                                                                                       |
| date_modified_ts          | дата время изменения, utc                                                                                                        |
| date_modified_dttm_local  | дата время изменения, local tz                                                                                                   |
| deleted                   | были ли удалена                                                                                                                  |
| crm_team_description_hash | описание команды                                                                                                                 |
| crm_team_id               | id команды, `PK`                                                                                                                 |
| modified_user_id          | id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) внесшего изменение, `FK` |
| crm_team_name_hash        | название команды                                                                                                                 |
| crm_team_name_2_hash      | название команды 2                                                                                                               |
| private                   | приватная?                                                                                                                       |
| segment                   | сегмент                                                                                                                          |


### Структура - PII

| Поле                 | Описание           |
|----------------------|--------------------|
| crm_team_id          | id команды, `PK`   |
| crm_team_description | описание команды   |
| crm_team_name        | название команды   |
| crm_team_name_2      | название команды 2 |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
