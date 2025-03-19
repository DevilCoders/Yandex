## crm team sets teams
#crm #crm_team_sets_teams

Таблица связывает команды и team_set.


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                                          | Источники                                                                                                                                       |
|---------|------------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [crm_team_sets_teams](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_team_sets_teams) | [raw-crm_team_sets_teams](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_team_sets_teams) |
| PREPROD | [crm_team_sets_teams](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_team_sets_teams) | [raw-crm_team_sets_teams](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_team_sets_teams) |


### Структура

| Поле                     | Описание                                                                                                      |
|--------------------------|---------------------------------------------------------------------------------------------------------------|
| date_modified_ts         | дата время модификации, utc                                                                                   |
| date_modified_dttm_local | дата время модификацииа, local tz                                                                             |
| deleted                  | была ли удалена                                                                                               |
| id                       | id                                                                                                            |
| crm_team_id              | id [primary_team](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_teams), `FK` |
| crm_team_set_id          | id team_set (команд мб несколько)                                                                             |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
