## crm notes
#crm #crm_notes

Содержит информацию заметках.


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Структура PII.](#структура_PII)
4. [Загрузка.](#загрузка)


### Расположение данных

| Контур    | Расположение данных   | Источники |
| --------- | -------------------   | --------- |
| PROD      | [crm_notes](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_notes), [PII-crm_notes](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/PII/crm_notes)  | [raw-crm_notes](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_notes) |
| PREPROD   | [crm_notes](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/crm/crm_notes), [PII-crm_notes](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/crm/PII/crm_notes)| [raw-crm_notes](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_notes) |


### Структура

| Поле                      | Описание                                                                                                                         |
|---------------------------|----------------------------------------------------------------------------------------------------------------------------------|
| acl_crm_team_set_id       | id team_set для acl (мб быть связано несколько команд)                                                                           |
| assigned_user_id          | присвоенный id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) , `FK`       |
| crm_contact_id            | id связанного [контакта](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_contacts), `FK`          |
| created_by                | создатель записи                                                                                                                 |
| date_entered_ts           | дата время ввода, utc                                                                                                            |
| date_entered_dttm_local   | дата время ввода, local tz                                                                                                       |
| date_modified_ts          | дата время модификации, utc                                                                                                      |
| date_modified_dttm_local  | дата время модификацииа, local tz                                                                                                |
| deleted                   | была ли заметка удалена                                                                                                          |
| crm_note_description_hash | описание, hash                                                                                                                   |
| crm_email_id              | id [email](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_email_addresses), `FK`                 |
| email_type                | типа emala                                                                                                                       |
| crm_note_id               | id заметки , `PK`                                                                                                                |
| modified_user_id          | id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) внесшего изменение, `FK` |
| crm_note_name             | имя заметки                                                                                                                      |
| parent_id                 | id родителя                                                                                                                      |
| parent_type               | тип родителя (лид, аккаунта, сделка)                                                                                             |
| crm_team_id               | id [primary_team](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_teams), `FK`                    |
| crm_team_set_id           | id team_set (мб быть связано несколько команд)                                                                                   |
| upload_id                 | id загрузки                                                                                                                      |


### Структура - PII

| Поле                 | Описание         |
|----------------------|------------------|
| crm_note_id          | id заметки, `PK` |
| crm_note_description | описание         |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
