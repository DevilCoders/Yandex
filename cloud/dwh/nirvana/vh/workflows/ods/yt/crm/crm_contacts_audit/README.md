## crm contacts audit
#crm #crm_contacts_audit

Содержит аудит по изменению данных о [контактах](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_contacts).


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Структура PII.](#структура_PII)
4. [Загрузка.](#загрузка)


### Расположение данных

| Контур    | Расположение данных   | Источники |
| --------- | -------------------   | --------- |
| PROD      | [crm_contacts_audit](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_contacts_audit), [PII-crm_contacts_audit](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/PII/crm_contacts_audit) | [raw-crm_contacts_audit](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_contacts_audit) |
| PREPROD   | [crm_contacts_audit](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/crm/crm_contacts_audit), [PII-crm_contacts_audit](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/crm/PII/crm_contacts_audit)| [raw-crm_contacts_audit](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_contacts_audit) |


### Структура

| Поле                                  | Описание                          |
|---------------------------------------|-----------------------------------|
| after_value_string                    | значение после изменения (строка) |
| after_value_text                      | значение после изменения (текст)  |
| before_value_string                   | значение до изменения (строка)    |
| before_value_text                     | значение до изменения (текст)     |
| created_by                            | кем создано изменение             |
| data_type                             | тип данных                        |
| date_created_ts                       | дата создания, utc                |
| date_created_dttm_local               | дата создания, local tz           |
| date_updated_ts - дата изменения, utc |
| date_updated_dttm_local               | дата изменения, local tz          |
| event_id                              | идентификатор события             |
| field_name                            | поле                              |
| id                                    | идентификатор изменения           |
| parent_id                             | идентификатор измения родителя    |


### Структура_PII

Содержит изменения стобцов:
- alt_address_city - дополнительный адрес, город
- alt_address_country - дополнительный адрес, страна
- alt_address_postalcode - дополнительный адрес, индекс
- alt_address_state - дополнительный адрес, государство
- alt_address_street - дополнительный адрес, улица
- assistant - ассистент
- assistant_phone - телефон ассистента
- birthdate - дата рождения
- facebook - facebook аккаунт
- first_name - Имя
- googleplus - googleplus аккаунт
- last_name - Фамилия
- linkedin - linkedin аккаунт
- phone_fax - факс
- phone_home - домашний телефон
- phone_mobile - мобильный телефон
- phone_other - другой телефон
- phone_work - рабочий телефон
- portal_password - пароль на портале
- primary_address_city - основной адрес, город
- primary_address_country - основной адрес,  страна
- primary_address_postalcode - основной адрес, индекс
- primary_address_state - основной адрес, государство
- primary_address_street - основной адрес, улица
- telegram_account - аккаунт telegram
- twitter - twitter аккаунт
- whatsapp_account - whatsapp аккаунт

| Поле                                  | Описание                          |
|---------------------------------------|-----------------------------------|
| after_value_string                    | значение после изменения (строка) |
| after_value_text                      | значение после изменения (текст)  |
| before_value_string                   | значение до изменения (строка)    |
| before_value_text                     | значение до изменения (текст)     |
| created_by                            | кем создано изменение             |
| data_type                             | тип данных                        |
| date_created_ts                       | дата создания, utc                |
| date_created_dttm_local               | дата создания, local tz           |
| date_updated_ts - дата изменения, utc |
| date_updated_dttm_local               | дата изменения, local tz          |
| event_id                              | идентификатор события             |
| field_name                            | поле                              |
| id                                    | идентификатор изменения           |
| parent_id                             | идентификатор измения родителя    |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
