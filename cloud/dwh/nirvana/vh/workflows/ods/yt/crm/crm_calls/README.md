## crm calls
#crm #crm_calls

Содержит информацию звонках.


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Структура PII.](#структура_PII)
4. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                                                                                                                                              | Источники                                                                                                                   |
|---------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------|
| PROD    | [crm_calls](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_calls), [PII-crm_calls](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/PII/crm_calls)       | [raw-crm_calls](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_calls) |
| PREPROD | [crm_calls](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/crm/crm_calls), [PII-crm_calls](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/crm/PII/crm_calls) | [raw-crm_calls](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_calls) |


### Структура

| Поле                              | Описание                                                                                                                         |
|-----------------------------------|----------------------------------------------------------------------------------------------------------------------------------|
| acl_crm_team_set_id               | id team_set (команд мб несколько)                                                                                                |
| assigned_user_id                  | присвоенный id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) , `FK`       |
| cj_actual_sort_order              |                                                                                                                                  |
| cj_momentum_end_date_ts           | , utc                                                                                                                            |
| cj_momentum_end_date_dttm_local   | , local tz                                                                                                                       |
| cj_momentum_points                |                                                                                                                                  |
| cj_momentum_ratio                 |                                                                                                                                  |
| cj_momentum_score                 |                                                                                                                                  |
| cj_momentum_start_date_ts         | , utc                                                                                                                            |
| cj_momentum_start_date_dttm_local | , local tz                                                                                                                       |
| cj_parent_activity_id             |                                                                                                                                  |
| cj_parent_activity_type           |                                                                                                                                  |
| cj_url                            |                                                                                                                                  |
| created_by                        |                                                                                                                                  |
| customer_journey_blocked_by       |                                                                                                                                  |
| customer_journey_points           |                                                                                                                                  |
| customer_journey_progress         |                                                                                                                                  |
| customer_journey_score            |                                                                                                                                  |
| date_click_to_call_ts             | , utc                                                                                                                            |
| date_click_to_call_dttm_local     | , local tz                                                                                                                       |
| date_end_ts                       | дата время окончания, utc                                                                                                        |
| date_end_dttm_local               | дата время окончания, local tz                                                                                                   |
| date_entered_ts                   | дата время входа, utc                                                                                                            |
| date_entered_dttm_local           | дата время входа, local tz                                                                                                       |
| date_modified_ts                  | дата время модификации, utc                                                                                                      |
| date_modified_dttm_local          | дата время модификации, local tz                                                                                                 |
| date_start_ts                     | дата время начала, utc                                                                                                           |
| date_start_dttm_local             | дата время начала, local tz                                                                                                      |
| deleted                           | был ли удален                                                                                                                    |
| crm_call_description              | описание по звонку                                                                                                               |
| direction                         | входящих или исходящий звонок                                                                                                    |
| dri_subworkflow_id                |                                                                                                                                  |
| dri_subworkflow_template_id       |                                                                                                                                  |
| dri_workflow_sort_order           |                                                                                                                                  |
| dri_workflow_task_template_id     |                                                                                                                                  |
| dri_workflow_template_id          |                                                                                                                                  |
| duration_hours                    | продолжительность звонка в часах                                                                                                 |
| duration_minutes                  | продолжительность звонка в минутах                                                                                               |
| email_reminder_sent               |                                                                                                                                  |
| email_reminder_time               |                                                                                                                                  |
| from_phone_hash                   | номер телефона с которого звонили, hash                                                                                          |
| history_duration                  |                                                                                                                                  |
| history_taked                     |                                                                                                                                  |
| crm_call_id                       | id звонка, `PK`                                                                                                                  |
| is_cj_parent_activity             |                                                                                                                                  |
| is_customer_journey_activity      |                                                                                                                                  |
| modified_user_id                  | id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) внесшего изменение, `FK` |
| crm_call_name                     | название звонка                                                                                                                  |
| outlook_id                        | id outlook                                                                                                                       |
| parent_id                         |                                                                                                                                  |
| parent_type                       |                                                                                                                                  |
| recurrence_id_ts                  | , utc                                                                                                                            |
| recurrence_id_dttm_local          | , local tz                                                                                                                       |
| recurring_source                  |                                                                                                                                  |
| reminder_time                     | время для напоминания                                                                                                            |
| repeat_count                      | количество повторов                                                                                                              |
| repeat_days                       | повторять раз в n дней                                                                                                           |
| repeat_dow                        |                                                                                                                                  |
| repeat_interval                   | интервал повтора                                                                                                                 |
| repeat_ordinal                    |                                                                                                                                  |
| repeat_parent_id                  |                                                                                                                                  |
| repeat_selector                   |                                                                                                                                  |
| repeat_type                       | тип повтора                                                                                                                      |
| repeat_unit                       | единица повтора                                                                                                                  |
| repeat_until                      | повторять до даты                                                                                                                |
| saved_manually                    | звонок сохранен вручную?                                                                                                         |
| crm_call_status                   | статус звонка                                                                                                                    |
| crm_team_id                       | id [primary_team](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_teams), `FK`                    |
| crm_team_set_id                   | id team_set (команд мб несколько)                                                                                                |
| to_phone_hash                     | номер телефона на который звонили, hash                                                                                          |


### Структура_PII

| Поле        | Описание                          |
|-------------|-----------------------------------|
| crm_call_id | id звонка, `PK`                   |
| from_phone  | номер телефона с которого звонили |
| to_phone    | номер телефона на который звонили |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
