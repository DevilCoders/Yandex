##### crm contacts
#crm #crm_contacts

Содержит контактную информацию.


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Структура PII.](#структура_PII)
4. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                                                                                                                                                          | Источники                                                                                                                         |
|---------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [crm_contacts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_contacts), [PII-crm_contacts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/PII/crm_contacts)       | [raw-crm_contacts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_contacts) |
| PREPROD | [crm_contacts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/crm/crm_contacts), [PII-crm_contacts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/crm/PII/crm_contacts) | [raw-crm_contacts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_contacts) |


### Структура

| Поле                                           | Описание                                                                                                                         |
|------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------|
| acl_crm_team_set_id                            | id team_set (команд может быть несколько)                                                                                        |
| alt_address_city_hash                          | дополнительный адрес, город, hash                                                                                                |
| alt_address_country_hash                       | дополнительный адрес, страна, hash                                                                                               |
| alt_address_postalcode_hash                    | дополнительный адрес, индекс, hash                                                                                               |
| alt_address_state_hash                         | дополнительный адрес, государство, hash                                                                                          |
| alt_address_street_hash                        | дополнительный адрес, улица, hash                                                                                                |
| assigned_user_id                               | присвоенный id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) , `FK`       |
| assistant_hash                                 | ассистент, hash                                                                                                                  |
| assistant_phone_hash                           | телефон ассистента, hash                                                                                                         |
| birthdate_hash                                 | дата рождения, hash                                                                                                              |
| campaign_id                                    | id компании                                                                                                                      |
| country_name_phone_mobile                      | страны мобильного телефона                                                                                                       |
| country_verified_phone_mobile                  | верифицирован ли мобильный телефон юзером?                                                                                       |
| created_by                                     | создано                                                                                                                          |
| date_entered                                   | дата время ввода, utc (удаление)                                                                                                 |
| date_entered_ts                                | дата время ввода, utc                                                                                                            |
| date_entered_dttm_local                        | дата время ввода, local tz                                                                                                       |
| date_modified                                  | дата время изменения,utc (удаление)                                                                                              |
| date_modified_ts                               | дата время изменения, utc                                                                                                        |
| date_modified_dttm_local                       | дата время изменения, local tz                                                                                                   |
| deleted                                        | была ли строка удалена                                                                                                           |
| department                                     | департамент                                                                                                                      |
| crm_contact_description                        | описание контакта                                                                                                                |
| dnb_principal_id                               |                                                                                                                                  |
| do_not_call                                    | не звонить?                                                                                                                      |
| do_not_mail                                    | не писать на email                                                                                                               |
| dp_business_purpose                            | бизнес цель                                                                                                                      |
| dp_consent_last_updated                        |                                                                                                                                  |
| dri_workflow_template_id -id шаблона  workflow |
| facebook_hash                                  | facebook аккаунт, hash                                                                                                           |
| first_name_hash                                | Имя, hash                                                                                                                        |
| googleplus_hash                                | googleplus аккаунт, hash                                                                                                         |
| crm_contact_id                                 | id контакта  , `PK`                                                                                                              |
| last_communication_date                        | дата последней коммуникации, utc (удаление)                                                                                      |
| last_communication_date_ts                     | дата последней коммуникации, utc                                                                                                 |
| last_communication_date_dttm_local             | дата последней коммуникации, local tz                                                                                            |
| last_name_hash                                 | Фамилия, hash                                                                                                                    |
| lead_source                                    | последний источник                                                                                                               |
| linkedin_hash                                  | linkedin аккаунт, hash                                                                                                           |
| modified_user_id                               | id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) внесшего изменение, `FK` |
| old_format_dimensions                          | старый формат измерений                                                                                                          |
| phone_fax_hash                                 | факс, hash                                                                                                                       |
| phone_home_hash                                | домашний телефон, hash                                                                                                           |
| phone_mobile_hash                              | мобильный телефон, hash                                                                                                          |
| phone_other_hash                               | другой телефон, hash                                                                                                             |
| phone_work_hash                                | рабочий телефон, hash                                                                                                            |
| picture                                        | id картинки                                                                                                                      |
| portal_active                                  | активен портал?                                                                                                                  |
| portal_app                                     |                                                                                                                                  |
| portal_name                                    | имя на портале                                                                                                                   |
| portal_password_hash                           | пароль на портале, hash                                                                                                          |
| preferred_language                             | предпочитаемый язык                                                                                                              |
| primary_address_city_hash                      | основной адрес, город, hash                                                                                                      |
| primary_address_country_hash                   | основной адрес,  страна, hash                                                                                                    |
| primary_address_postalcode_hash                | основной адрес, индекс, hash                                                                                                     |
| primary_address_state_hash                     | основной адрес, государство, hash                                                                                                |
| primary_address_street_hash                    | основной адрес, улица, hash                                                                                                      |
| reports_to_id                                  | кому подчиняется контакт (если применимо)?                                                                                       |
| salutation                                     | приветствие                                                                                                                      |
| crm_team_id                                    | id [primary_team](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_teams), `FK`                    |
| crm_team_set_id                                | id team_set (команд мб несколько)                                                                                                |
| telegram_account_hash                          | аккаунт telegram, hash                                                                                                           |
| timezone                                       | таймзона                                                                                                                         |
| title                                          | заголовок                                                                                                                        |
| twitter_hash                                   | twitter аккаунт, hash                                                                                                            |
| whatsapp_account_hash                          | whatsapp аккаунт, hash                                                                                                           |


### Структура_PII

| Поле                       | Описание                          |
|----------------------------|-----------------------------------|
| crm_contact_id             | id контакта , `PK`                |
| alt_address_city           | дополнительный адрес, город       |
| alt_address_country        | дополнительный адрес, страна      |
| alt_address_postalcode     | дополнительный адрес, индекс      |
| alt_address_state          | дополнительный адрес, государство |
| alt_address_street         | дополнительный адрес, улица       |
| assistant                  | ассистент                         |
| assistant_phone            | телефон ассистента                |
| birthdate                  | дата рождения                     |
| facebook                   | facebook аккаунт                  |
| first_name                 | Имя                               |
| googleplus                 | googleplus аккаунт                |
| last_name                  | Фамилия                           |
| linkedin                   | linkedin аккаунт                  |
| phone_fax                  | факс                              |
| phone_home                 | домашний телефон                  |
| phone_mobile               | мобильный телефон                 |
| phone_other                | другой телефон                    |
| phone_work                 | рабочий телефон                   |
| portal_password            | пароль на портале                 |
| primary_address_city       | основной адрес, город             |
| primary_address_country    | основной адрес,  страна           |
| primary_address_postalcode | основной адрес, индекс            |
| primary_address_state      | основной адрес, государство       |
| primary_address_street     | основной адрес, улица             |
| telegram_account           | аккаунт telegram                  |
| twitter                    | twitter аккаунт                   |
| whatsapp_account           | whatsapp аккаунт                  |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
