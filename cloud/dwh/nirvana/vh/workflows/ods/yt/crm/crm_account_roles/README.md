## crm account roles
#crm #crm_account_roles

Содержит информацию о ролях в аккаунте.


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Структура PII.](#структура_PII)
4. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                                                                                                                                                                              | Источники                                                                                                                                  |
|---------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [crm_account_roles](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_account_roles), [PII-crm_account_roles](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/PII/crm_account_roles)       | [raw-crm_account_roles](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_accountroles) |
| PREPROD | [crm_account_roles](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/crm/crm_account_roles), [PII-crm_account_roles](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/crm/PII/crm_account_roles) | [raw-crm_account_roles](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_accountroles) |


### Структура

| Поле                         | Описание                                                                                                                                   |
|------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------|
| crm_account_id               | id [аккаунт в CRM](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_accounts), которому присвоена роль, `FK` |
| assigned_user_id             | id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) , которому присвоена роль, `FK`    |
| created_by                   | создано                                                                                                                                    |
| date_entered_ts              | дата время ввода, utc                                                                                                                      |
| date_entered_dttm_local      | дата время ввода, local tz                                                                                                                 |
| date_from                    | начало действия роли, utc (удаление)                                                                                                       |
| date_from_ts                 | начало действия роли, utc                                                                                                                  |
| date_from_dttm_local         | начало действия роли, local tz                                                                                                             |
| date_modified_ts             | дата время изменения, utc                                                                                                                  |
| date_modified_dttm_local     | дата время изменения, local tz                                                                                                             |
| date_to                      | конец действия роли, utc (удаление)                                                                                                        |
| date_to_ts                   | конец действия роли, utc                                                                                                                   |
| date_to_dttm_local           | конец действия роли, local tz                                                                                                              |
| deleted                      | была ли роль удалена                                                                                                                       |
| crm_account_role_description | описание роли аккаунта                                                                                                                     |
| crm_account_role_id          | id роли аккаунта , `PK`                                                                                                                    |
| last_confirmator_id          | id последнего утверждающего                                                                                                                |
| modified_user_id             | id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) внесшего изменение, `FK`           |
| crm_account_role_name_hash   | название роли аккаунта, hash                                                                                                               |
| requester_id                 | id запроса                                                                                                                                 |
| crm_role_name                | имя роли                                                                                                                                   |
| segment_id                   | id [сегмента](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_segments), `FK`                               |
| share                        |                                                                                                                                            |
| share_custom_audit           |                                                                                                                                            |
| status                       | статус роли                                                                                                                                |


### Структура_PII

| Поле                  | Описание               |
|-----------------------|------------------------|
| crm_account_role_id   | id роли аккаунта, `PK` |
| crm_account_role_name | название роли аккаунта |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
