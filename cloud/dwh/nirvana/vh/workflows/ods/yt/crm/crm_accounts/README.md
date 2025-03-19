## crm accounts
#crm #crm_accounts

Содержит информацию об аккаунте.


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Структура PII.](#структура_PII)
4. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                                                                                                                                                          | Источники                                                                                                                         |
|---------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [crm_accounts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_accounts), [PII-crm_accounts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/PII/crm_accounts)       | [raw-crm_accounts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_accounts) |
| PREPROD | [crm_accounts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/crm/crm_accounts), [PII-crm_accounts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/crm/PII/crm_accounts) | [raw-crm_accounts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_accounts) |


### Структура

| Поле                             | Описание                                                                                                                         |
|----------------------------------|----------------------------------------------------------------------------------------------------------------------------------|
| account_type                     | тип аккаунта                                                                                                                     |
| acl_crm_team_set_id              | id team_set (команд может быть несколько)                                                                                        | FK
| annual_revenue                   |                                                                                                                                  |
| assigned_user_id                 | присвоенный id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users), `FK`        |
| assigned_user_id_second          | присвоенный id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users), `FK`        |
| assigned_user_second_share       |                                                                                                                                  |
| assigned_user_share              |                                                                                                                                  |
| ba_base_rate                     | rate валюты сделки                                                                                                               |
| ba_currency_id                   | id валюты из связанного ba                                                                                                       |
| crm_balance_id                   | id баланса                                                                                                                       |
| billing_address_city             | платежный адрес, город                                                                                                           |
| billing_address_country_hash     | платежный адрес, страна, hash                                                                                                    |
| billing_address_postalcode_hash  | платежный адрес, индекс, hash                                                                                                    |
| billing_address_state_hash       | платежный адрес, государство, hash                                                                                               |
| billing_address_street_hash      | платежный адрес, улица, hash                                                                                                     |
| billing_counterparty_id          |                                                                                                                                  |
| block_reason                     | причина блокировки                                                                                                               |
| crm_campaign_id                  | id компании                                                                                                                      |
| cloud_budget                     | бюджет в обалаке                                                                                                                 |
| cloudboost_account               | аккаунт cloudboost                                                                                                               |
| consumption_type                 | тип потребления                                                                                                                  |
| country_directory                | страна                                                                                                                           |
| country_name_phone_alternate     | название страны другого телефона                                                                                                 |
| country_name_phone_office        | название страны телефона офиса                                                                                                   |
| country_verified_phone_alternate | проверен другой телефон                                                                                                          |
| country_verified_phone_office    | проверен телефон офиса                                                                                                           |
| created_by                       | создано                                                                                                                          |
| crm_currency_id                  | id валюты                                                                                                                        |
| customer_interest_level          | уровень заинтересованности клиента                                                                                               |
| date_entered_ts                  | дата время ввода, utc                                                                                                            |
| date_entered_dttm_local          | дата время ввода, local tz                                                                                                       |
| date_modified_ts                 | дата время изменения, utc                                                                                                        |
| date_modified_dttm_local         | дата время изменения, local tz                                                                                                   |
| deleted                          | был ли аккаунт удален                                                                                                            |
| crm_account_description_hash     | описание аккаунта в CRM, hash                                                                                                    |
| display_status                   | отображаемый статус                                                                                                              |
| dri_workflow_template_id         |                                                                                                                                  |
| duns_num                         |                                                                                                                                  |
| employees                        | количество работников                                                                                                            |
| facebook_hash                    | аккаунт в facebook, hash                                                                                                         |
| first_name_hash                  | Имя, hash                                                                                                                        |
| first_trial_consumption_date     |                                                                                                                                  |
| full_name                        | полное имя                                                                                                                       |
| googleplus_hash                  | аккаунт в googleplus, hash                                                                                                       |
| crm_account_id                   | внутренний id аккаунта в CRM , `PK`                                                                                              |
| industry - индустрия             |
| inn                              | ИНН                                                                                                                              |
| internal_ent_segment             | внутренняя сегментация Enterprise команды                                                                                        |
| kpp                              | КПП                                                                                                                              |
| last_name_hash                   | Фамилия, hash                                                                                                                    |
| linked_total_billing_accounts    | связано всего платежных аккаунтов                                                                                                |
| linked_total_contacts            | связано всего контактов                                                                                                          |
| linked_total_leads               | связано всего заявок                                                                                                             |
| linked_total_notes               | связано всего заметок                                                                                                            |
| linked_total_opportunities       | связано всего возможностей                                                                                                       |
| linked_total_tasks               | связано всего задач                                                                                                              |
| main_ba_id                       | ID основного биллинг-аккаунта клиента (если применимо)                                                                           |
| modified_user_id                 | id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) внесшего изменение, `FK` |
| crm_account_name                 | название аккаунта из CRM                                                                                                         |
| old_format_dimensions            | старый формат измерений                                                                                                          |
| org_type                         | тип организации                                                                                                                  |
| ownership                        | владелец                                                                                                                         |
| paid_consumption                 | оплаченное потребление                                                                                                           |
| parent_id                        | id родителя                                                                                                                      |
| person_type                      | тип родителя                                                                                                                     |
| phone_alternate_hash             | другой телефон, hash                                                                                                             |
| phone_fax_hash                   | факс, hash                                                                                                                       |
| phone_office_hash                | телефон офиса, hash                                                                                                              |
| rating                           | рейтинг                                                                                                                          |
| second_owner                     | второй владелец                                                                                                                  |
| segment                          | сегмент                                                                                                                          |
| segment_ba                       | сегмент связанного биллинг-аккаунта                                                                                              |
| crm_segment_id                   | id [сегмента](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_segments), `FK`                     |
| segment_manually                 | признак, что выполнен override сегмента у клиента                                                                                |
| service_rang                     | уровень поддержки (бесплатная, бизнес, премиум)                                                                                  |
| shipping_address_city            | адрес доставки, город                                                                                                            |
| shipping_address_country_hash    | адрес доставки, страна, hash                                                                                                     |
| shipping_address_postalcode_hash | адрес доставки, индекс, hash                                                                                                     |
| shipping_address_state_hash      | адрес доставки, государство, hash                                                                                                |
| shipping_address_street_hash     | адрес доставки, улица, hash                                                                                                      |
| sic_code                         | SIC код клиента                                                                                                                  |
| sub_industry                     | подгруппа индустрии                                                                                                              |
| crm_team_id                      | id [primary_team](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_teams), `FK`                    |
| crm_team_set_id                  | id team_set (команд может быть несколько)                                                                                        |
| ticker_symbol                    | код компании на бирже                                                                                                            |
| timezone                         | таймзона                                                                                                                         |
| trial_consumption                | триальное потребление                                                                                                            |
| twitter_hash                     | аккаунт в twitter, hash                                                                                                          |
| website                          | url                                                                                                                              |

### Структура_PII

| Поле                        | Описание                            |
|-----------------------------|-------------------------------------|
| crm_account_id              | внутренний id аккаунта в CRM , `PK` |
| crm_account_description     | описание аккаунта в CRM             |
| billing_address_country     | платежный адрес, страна             |
| billing_address_postalcode  | платежный адрес, индекс             |
| billing_address_state       | платежный адрес, государство        |
| billing_address_street      | платежный адрес, улица              |
| facebook                    | аккаунт в facebook                  |
| first_name                  | Имя                                 |
| googleplus                  | аккаунт в googleplus                |
| last_name                   | Фамилия                             |
| phone_alternate             | другой телефон                      |
| phone_fax                   | факс                                |
| phone_office                | телефон офиса                       |
| shipping_address_country    | адрес доставки, страна              |
| shipping_address_postalcode | адрес доставки, индекс              |
| shipping_address_state      | адрес доставки, государство         |
| shipping_address_street     | адрес доставки, улица               |
| twitter                     | аккаунт в twitter                   |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
