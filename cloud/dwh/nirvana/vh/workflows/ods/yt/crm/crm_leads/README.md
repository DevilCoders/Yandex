## crm leads
#crm #crm_leads

Содержит информацию о лидах.


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Структура PII.](#структура_PII)
4. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                                                                                                                                              | Источники                                                                                                                   |
|---------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------|
| PROD    | [crm_leads](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_leads), [PII-crm_leads](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/PII/crm_leads)       | [raw-crm_leads](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_leads) |
| PREPROD | [crm_leads](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/crm/crm_leads), [PII-crm_leads](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/crm/PII/crm_leads) | [raw-crm_leads](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_leads) |


### Структура

| Поле                                   | Описание                                                                                                                         |
|----------------------------------------|----------------------------------------------------------------------------------------------------------------------------------|
| ac_full_name                           |                                                                                                                                  |
| crm_account_description                | описание аккаунта в CRM                                                                                                          |
| crm_account_id                         | id [аккаунта в CRM](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_accounts), `FK`               |
| crm_account_name_hash                  | название аккаунта в CRM,hash                                                                                                     |
| acl_crm_team_set_id                    | id team_set (команд может быть несколько)                                                                                        | FK
| allow_restore                          | был ли лид возвращен из терминальной стадии?                                                                                     |
| alt_address_city                       | дополнительный адрес, город                                                                                                      |
| alt_address_country_hash               | дополнительный адрес, страна, hash                                                                                               |
| alt_address_postalcode_hash            | дополнительный адрес, индекс, hash                                                                                               |
| alt_address_state_hash                 | дополнительный адрес, государство, hash                                                                                          |
| alt_address_street_hash                | дополнительный адрес, улица, hash                                                                                                |
| assigned_user_id                       | присвоенный id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) , `FK`       |
| assistant_hash                         | ассистент, hash                                                                                                                  |
| assistant_phone_hash                   | телефон ассистента, hash                                                                                                         |
| ba_base_rate                           | rate валюты из связанного с лидом ba (системное поле)                                                                            |
| ba_currency_id                         | id валюты из связанного с лидом лида (системное поле)                                                                            |
| base_rate                              | rate валюты лида (системное поле)                                                                                                |
| billing_address_city                   | адрес для выставления счетов, город                                                                                              |
| billing_address_country_hash           | адрес для выставления счетов, страна, hash                                                                                       |
| billing_address_postalcode_hash        | адрес для выставления счетов, индекс, hash                                                                                       |
| billing_address_state_hash             | адрес для выставления счетов, государство, hash                                                                                  |
| billing_address_street_hash            | адрес для выставления счетов, улица, hash                                                                                        |
| birthdate_hash                         | дата рождения, hash                                                                                                              |
| block_reason                           | причина блокировки                                                                                                               |
| callback_date_ts                       | дата обратной связи, utc                                                                                                         |
| callback_date_dttm_local               | дата обратной связи, local tz                                                                                                    |
| crm_campaign_id                        | id компании                                                                                                                      |
| client_request_type                    | тип запроса клиента                                                                                                              |
| cloud_iam_id                           | iam id                                                                                                                           |
| cloud_id_project                       | id проекта в облаке                                                                                                              |
| cloud_name                             | название облака                                                                                                                  |
| consumption_type                       | тип потребления                                                                                                                  |
| crm_contact_id                         | id [контакта](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_contacts), `FK`                     |
| converted                              | сконвертирован?                                                                                                                  |
| country_directory                      | страна поставки                                                                                                                  |
| country_name_phone_mobile              | страна моб телефона                                                                                                              |
| country_verified_phone_mobile          | проверен ли моб телефон                                                                                                          |
| created_by                             | созданный                                                                                                                        |
| crm_currency_id                        | id валюты                                                                                                                        |
| data_from_unsuccessful_attempt         |                                                                                                                                  |
| data_from_unsuccessful_attempt_support |                                                                                                                                  |
| date_entered_ts                        | дата время ввода, utc                                                                                                            |
| date_entered_dttm_local                | дата время ввода, local tz                                                                                                       |
| date_modified_ts                       | дата время модификации, utc                                                                                                      |
| date_modified_dttm_local               | дата время модификации, local tz                                                                                                 |
| date_modified_from_yt_ts               | модификация сделана в yt, utc                                                                                                    |
| date_modified_from_yt_dttm_local       | модификация сделана в yt, local tz                                                                                               |
| deleted                                | удален ли информация                                                                                                             |
| department                             | департамент                                                                                                                      |
| crm_lead_description                   | описание лида                                                                                                                    |
| discount                               | скидка                                                                                                                           |
| display_status                         | отображаемый статус                                                                                                              |
| disqualified_reason                    | причина дисквалификации (список)                                                                                                 |
| disqualified_reason_description        | описание причины дисквалификации                                                                                                 |
| do_not_call                            | не звонить                                                                                                                       |
| do_not_mail                            | не писать                                                                                                                        |
| employees                              | кол-во работников                                                                                                                |
| facebook_hash                          | id facebook, hash                                                                                                                |
| first_name_hash                        | Имя, hash                                                                                                                        |
| first_trial_consumption_date           | дата первого бесплатного периода                                                                                                 |
| folder_id_project                      | id папки проекта                                                                                                                 |
| googleplus_hash                        | googleplus, hash                                                                                                                 |
| iam_id                                 | id iam                                                                                                                           |
| crm_lead_id                            | id лида , `PK`                                                                                                                   |
| industry                               | сфера деятельности                                                                                                               |
| inn                                    | ИНН                                                                                                                              |
| last_communication_date_ts             | дата последней коммуникации, utc                                                                                                 |
| last_communication_date_dttm_local     | дата последней коммуникации, local tz                                                                                            |
| last_name_hash                         | Фамилия, hash                                                                                                                    |
| lead_priority                          | приоритет лида                                                                                                                   |
| lead_source                            | источник лида                                                                                                                    |
| lead_source_description                | описание источника лида                                                                                                          |
| lead_weight                            | вес лида                                                                                                                         |
| linked_total_billing_accounts          | кол-во связанных платежных аккаунтов                                                                                             |
| linked_total_calls                     | кол-во связанных звонков                                                                                                         |
| linked_total_emails                    | кол-во связанных emails                                                                                                          |
| linked_total_notes                     | кол-во связанных заметок                                                                                                         |
| linked_total_tasks                     | кол-во связанных задача                                                                                                          |
| modified_user_id                       | id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) внесшего изменение, `FK` |
| old_format_dimensions                  | старый формат измерений                                                                                                          |
| opportunity_amount                     | возможная сумма                                                                                                                  |
| opportunity_amount_currency            | возможная сумма в валюте                                                                                                         |
| opportunity_date_closed                | возможная дата сделки                                                                                                            |
| opportunity_date_closed_month          | возможный месяц сделки                                                                                                           |
| crm_opportunity_id                     | id [сделки](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_opportunities), `FK`                  |
| opportunity_likely                     | вероятная сделки                                                                                                                 |
| opportunity_likely_usdollar            | вероятная сделки в долларах                                                                                                      |
| opportunity_name                       | название сделки                                                                                                                  |
| org_type                               | тип ораганизации                                                                                                                 |
| paid_consumption                       | оплачиваемое потребление                                                                                                         |
| crm_partner_id                         | id партнера                                                                                                                      |
| passport_uid                           | id паспорта                                                                                                                      |
| person_type                            | тип клиента (физик, юрик и тд)                                                                                                   |
| phone_fax_hash                         | факс, hash                                                                                                                       |
| phone_home_hash                        | домашний телефон, hash                                                                                                           |
| phone_mobile_hash                      | мобильный телефон, hash                                                                                                          |
| phone_other_hash                       | другой телефон, hash                                                                                                             |
| phone_work_hash                        | телефон рабочий, hash                                                                                                            |
| picture                                | id картинки                                                                                                                      |
| portal_app -                           |
| portal_name                            |                                                                                                                                  |
| preferred_language                     | предпочитаемый язык                                                                                                              |
| primary_address_city                   | основной адрес, город                                                                                                            |
| primary_address_country_hash           | основной адрес,  страна, hash                                                                                                    |
| primary_address_postalcode_hash        | основной адрес, индекс, hash                                                                                                     |
| primary_address_state_hash             | основной адрес, государство, hash                                                                                                |
| primary_address_street_hash            | основной адрес, улица, hash                                                                                                      |
| project_end_date                       | дата окончания проекта                                                                                                           |
| project_start_date                     | дата начала проекта                                                                                                              |
| promocode                              | промокод                                                                                                                         |
| promocode_activation_date              | дата активации промокода                                                                                                         |
| promocode_sum                          | сумма промокода                                                                                                                  |
| refered_by                             | упоминаемый                                                                                                                      |
| release_date_consumption               | дата окончания тестовое периода                                                                                                  |
| reports_to_id                          | id родительского лида                                                                                                            |
| salutation_hash                        | приветствие, hash                                                                                                                |
| scoring_results                        | результат скоринга                                                                                                               |
| segment_ba                             | сегмент связанного биллинг-аккаунта                                                                                              |
| send_to_isv_team                       | был ли лид передан в команду isv?                                                                                                |
| shipping_address_city                  | адрес доставки, город                                                                                                            |
| shipping_address_country_hash          | адрес доставки, страна, hash                                                                                                     |
| shipping_address_postalcode_hash       | адрес доставки, индекс, hash                                                                                                     |
| shipping_address_state_hash            | адрес доставки, государство, hash                                                                                                |
| shipping_address_street_hash           | адрес доставки, улица, hash                                                                                                      |
| status                                 | статус                                                                                                                           |
| status_description                     | статус описание                                                                                                                  |
| synced_with_main_ba                    |                                                                                                                                  |
| crm_team_id                            | id [primary_team](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_teams), `FK`                    |
| crm_team_set_id                        | id team_set (команд может быть несколько)                                                                                        |
| timezone                               | таймзона                                                                                                                         |
| title_hash                             | должность, hash                                                                                                                  |
| tracker_id                             | id задачи в трекере(не заполняется)                                                                                              |
| tracker_number                         | номер задачи в трекере                                                                                                           |
| training_status                        | статус задачи в трекере                                                                                                          |
| trial_consumption                      | тестовое потребление                                                                                                             |
| twitter_hash                           | аккаунт в twitter, hash                                                                                                          |
| utm_campaign                           | название рекламной кампании                                                                                                      |
| utm_content                            | дополнительная информация, которая помогает различать объявления                                                                 |
| utm_medium                             | тип трафика                                                                                                                      |
| utm_source                             | источник перехода                                                                                                                |
| utm_term                               | ключевая фраза                                                                                                                   |
| was_pending                            | находился ли в статусе pending ранее (возможно только один раз)                                                                  |
| was_round_robin                        | был ли автоматически распределен (возможно только один раз)                                                                      |
| website                                | адрес сайта                                                                                                                      |
| whatsapp_message_sent                  | отправлено ли сообщение в whatsapp                                                                                               |
| yandex_login_hash                      | логин яндекса, hash                                                                                                              |


### Структура_PII

| Поле                        | Описание                                  |
|-----------------------------|-------------------------------------------|
| crm_lead_id                 | id лида , `PK`                            |
| account_name                | название аккаунта                         |
| alt_address_city            | дополнительный адрес, город               |
| alt_address_country         | дополнительный адрес, страна              |
| alt_address_postalcode      | дополнительный адрес, индекс              |
| alt_address_state           | дополнительный адрес, государство         |
| alt_address_street          | дополнительный адрес, улица               |
| assistant                   | ассистент                                 |
| assistant_phone             | телефон ассистента                        |
| billing_address_country     | адрес для выставления счетов, страна      |
| billing_address_postalcode  | адрес для выставления счетов, индекс      |
| billing_address_state       | адрес для выставления счетов, государство |
| billing_address_street      | адрес для выставления счетов, улица       |
| birthdate                   | дата рождения                             |
| facebook                    | id facebook                               |
| first_name                  | Имя                                       |
| googleplus                  | googleplus                                |
| last_name                   | Фамилия                                   |
| phone_fax                   | факс                                      |
| phone_home                  | домашний телефон                          |
| phone_mobile                | мобильный телефон                         |
| phone_other                 | другой телефон                            |
| phone_work                  | телефон рабочий                           |
| primary_address_country     | основной адрес,  страна                   |
| primary_address_postalcode  | основной адрес, индекс                    |
| primary_address_state       | основной адрес, государство               |
| primary_address_street      | основной адрес, улица                     |
| salutation                  | приветствие                               |
| shipping_address_country    | адрес доставки, страна                    |
| shipping_address_postalcode | адрес доставки, индекс                    |
| shipping_address_state      | адрес доставки, государство               |
| shipping_address_street     | адрес доставки, улица                     |
| title                       | должность                                 |
| twitter                     | аккаунт в twitter                         |
| yandex_login                | логин яндекса                             |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
