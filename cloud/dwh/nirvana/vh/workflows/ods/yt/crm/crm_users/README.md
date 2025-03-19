## crm users
#crm #crm_users

Содержит информацию о пользователях.


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Структура PII.](#структура_PII)
4. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                                                                                                                                              | Источники                                                                                                                   |
|---------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------|
| PROD    | [crm_users](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_users), [PII-crm_users](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/PII/crm_users)       | [raw-crm_users](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_users) |
| PREPROD | [crm_users](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/crm/crm_users), [PII-crm_users](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/crm/PII/crm_users) | [raw-crm_users](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_users) |


### Структура

| Поле                          | Описание                                                                                                                         |
|-------------------------------|----------------------------------------------------------------------------------------------------------------------------------|
| acl_role_set_id               |                                                                                                                                  |
| acl_crm_team_set_id           | id team_set (команд может быть несколько)                                                                                        |
| add_to_bcc_for_outgoing_email |                                                                                                                                  |
| address_city                  | адрес, город                                                                                                                     |
| address_country_hash          | адрес, страна, hash                                                                                                              |
| address_postalcode_hash       | адрес, индекс, hash                                                                                                              |
| address_state_hash            | адрес, государство, hash                                                                                                         |
| address_street_hash           | адрес, улица, hash                                                                                                               |
| authenticate_id               |                                                                                                                                  |
| created_by                    | создано                                                                                                                          |
| date_entered_ts               | дата время ввода, utc                                                                                                            |
| date_entered_dttm_local       | дата время ввода, local tz                                                                                                       |
| date_modified_ts              | дата время модификации, utc                                                                                                      |
| date_modified_dttm_local      | дата время модификацииа, local tz                                                                                                |
| default_team                  | команда по умолчанию                                                                                                             |
| deleted                       | был ли пользователь удален                                                                                                       |
| department                    | департамент                                                                                                                      |
| crm_user_description          | описание пользователя                                                                                                            |
| employee_status               | статус работника                                                                                                                 |
| external_auth_only            | только внешняя аутентификация                                                                                                    |
| first_name_hash               | имя, hash                                                                                                                        |
| first_name_en_hash            | имя на англ, hash                                                                                                                |
| crm_user_id                   | id пользователя , `PK`                                                                                                           |
| import_email_from_robot       |                                                                                                                                  |
| internal_phone_hash           | внутренний номер                                                                                                                 |
| is_admin                      | админ?                                                                                                                           |
| is_group                      | группа?                                                                                                                          |
| last_login_ts                 | дата последнего входа, utc                                                                                                       |
| last_login_dttm_local         | дата последнего входа, local tz                                                                                                  |
| last_name_hash                | фамилия, hash                                                                                                                    |
| last_name_en_hash             | фамилия на англ, hash                                                                                                            |
| crm_messenger_id              | id месседжера                                                                                                                    |
| messenger_type                | тип сообщения                                                                                                                    |
| modified_user_id              | id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) внесшего изменение, `FK` |
| phone_fax_hash                | факс, hash                                                                                                                       |
| phone_home_hash               | домашний телефон, hash                                                                                                           |
| phone_mobile_hasр             | телефон мобильный, hash                                                                                                          |
| phone_other_hash              | другой телефон, hash                                                                                                             |
| phone_work_hash               | телефон рабочий, hash                                                                                                            |
| preferred_language            | предпочитаемый язык                                                                                                              |
| pwd_last_changed_ts           | дата последней смены пароля, utc                                                                                                 |
| pwd_last_changed_dttm_local   | дата последней смены пароля, local tz                                                                                            |
| receive_import_notifications  |                                                                                                                                  |
| receive_notifications         | получать уведомения?                                                                                                             |
| reports_to_id                 | id пользователя, которому подчиняется юзер                                                                                       |
| show_popup_card               | открывается ли у юзера popup при click-to-call?                                                                                  |
| status                        | статус                                                                                                                           |
| sugar_login                   |                                                                                                                                  |
| crm_team_set_id               | id team_set (команд может быть несколько)                                                                                        |
| title                         | должность                                                                                                                        |
| title_en                      | должность на англ                                                                                                                |
| use_click_to_call             | использует ли юзер click-to-call?                                                                                                |
| crm_user_name                 | имя пользователя                                                                                                                 |


### Структура_PII

| Поле               | Описание               |
|--------------------|------------------------|
| crm_user_id        | id пользователя , `PK` |
| address_country    | адрес, страна          |
| address_postalcode | адрес, индекс          |
| address_state      | адрес, государство     |
| address_street     | адрес, улица           |
| first_name         | имя                    |
| first_name_en      | имя на англ            |
| internal_phone     | внутренний номер       |
| last_name          | фамилия                |
| last_name_en       | фамилия на англ        |
| phone_fax          | факс                   |
| phone_home         | домашний телефон       |
| phone_mobile       | телефон мобильный      |
| phone_other        | другой телефон         |
| phone_work         | телефон рабочий        |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
