## crm_tasks
#crm #crm_tasks

Содержит информацию о задачах.


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Структура PII.](#структура-PII)
4. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                                                                                                                                              | Источники                                                                                                                   |
|---------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------|
| PROD    | [crm_tasks](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_tasks), [PII-crm_tasks](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/PII/crm_tasks)       | [raw-crm_tasks](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_tasks) |
| PREPROD | [crm_tasks](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/crm/crm_tasks), [PII-crm_tasks](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/crm/PII/crm_tasks) | [raw-crm_tasks](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_tasks) |


### Структура

| Поле                      | Описание                                                                                                                         |
|---------------------------|----------------------------------------------------------------------------------------------------------------------------------|
| acl_crm_team_set_id       | id team_set (команд мб несколько)                                                                                                |
| assigned_user_id          | присвоенный id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) , `FK`       |
| crm_contact_id            | id [контакта](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_contacts), `FK`                     |
| created_by                | созданный                                                                                                                        |
| date_due_ts               | дата выполнения, utc                                                                                                             |
| date_due_dttm_local       | дата выполнения, local tz                                                                                                        |
| date_entered_ts           | дата время ввода, utc                                                                                                            |
| date_entered_dttm_local   | дата время ввода, local tz                                                                                                       |
| date_modified_ts          | дата время изменения, utc                                                                                                        |
| date_modified_dttm_local  | дата время изменения, local tz                                                                                                   |
| date_start_ts             | дата начала, utc                                                                                                                 |
| date_start_dttm_local     | дата начала, local tz                                                                                                            |
| deleted                   | удалена ли задача                                                                                                                |
| crm_task_description_hash | описание таска, hash                                                                                                             |
| crm_task_id               | id таска                                                                                                                         |
| modified_user_id          | id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) внесшего изменение, `FK` |
| crm_task_name             | название задачи                                                                                                                  |
| parent_id                 | id родителя                                                                                                                      |
| parent_type               | тип родителя (лид, сделка, аккаунт и тд)                                                                                         |
| priority                  | приоритет                                                                                                                        |
| status                    | статус                                                                                                                           |
| crm_team_id               | id [primary_team](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_teams), `FK`                    |
| crm_team_set_id           | id team_set (команд мб несколько)                                                                                                |
| tracker_number            | номер в трекере                                                                                                                  |
| type                      | тип                                                                                                                              |


### Структура_PII

| Поле                 | Описание       |
|----------------------|----------------|
| crm_task_id          | id таска       |
| crm_task_description | описание таска |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
