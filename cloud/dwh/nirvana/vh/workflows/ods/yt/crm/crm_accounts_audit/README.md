## crm accounts audit
#crm #crm_accounts_audit

Содержит аудит по изменению данных [аккаунта в CRM](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_accounts).


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Структура PII.](#структура_PII)
4. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                                                                                                                                                                                  | Источники                                                                                                                                     |
|---------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [crm_accounts_audit](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_accounts_audit), [PII-crm_accounts_audit](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/PII/crm_accounts_audit)       | [raw-crm_accounts_audit](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_accounts_audit) |
| PREPROD | [crm_accounts_audit](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/crm/crm_accounts_audit), [PII-crm_accounts_audit](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/crm/PII/crm_accounts_audit) | [raw-crm_accounts_audit](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_accounts_audit) |


### Структура

| Поле                    | Описание                          |
|-------------------------|-----------------------------------|
| after_value_string      | значение после изменения (строка) |
| after_value_text        | значение после изменения (текст)  |
| before_value_string     | значение до изменения (строка)    |
| before_value_text       | значение до изменения (текст)     |
| created_by              | кем создано изменение             |
| data_type               | тип данных                        |
| date_created_ts         | дата создания, utc                |
| date_created_dttm_local | дата создания, local tz           |
| date_updated_ts         | дата изменения, utc               |
| date_updated_dttm_local | дата изменения, local tz          |
| event_id                | идентификатор события             |
| field_name              | поле                              |
| id                      | идентификатор изменения           |
| parent_id               | идентификатор измения родителя    |


### Структура_PII

Содержит изменения стобцов:
- crm_account_description - описание аккаунта в CRM
- billing_address_country - платежный адрес, страна
- billing_address_postalcode - платежный адрес, индекс
- billing_address_state - платежный адрес, государство
- billing_address_street - платежный адрес, улица
- facebook - аккаунт в facebook
- first_name - Имя
- googleplus - аккаунт в googleplus
- last_name - Фамилия
- crm_account_name - название аккаунта в CRM
- phone_alternate - другой телефон
- phone_fax - факс
- phone_office - телефон офиса
- shipping_address_country - адрес доставки, страна
- shipping_address_postalcode - адрес доставки, индекс
- shipping_address_state - адрес доставки, государство
- shipping_address_street - адрес доставки, улица
- twitter - аккаунт в twitter

| Поле                    | Описание                          |
|-------------------------|-----------------------------------|
| after_value_string      | значение после изменения (строка) |
| after_value_text        | значение после изменения (текст)  |
| before_value_string     | значение до изменения (строка)    |
| before_value_text       | значение до изменения (текст)     |
| created_by              | кем создано изменение             |
| data_type               | тип данных                        |
| date_created_ts         | дата создания, utc                |
| date_created_dttm_local | дата создания, local tz           |
| date_updated_ts         | дата изменения, utc               |
| date_updated_dttm_local | дата изменения, local tz          |
| event_id                | идентификатор события             |
| field_name              | поле                              |
| id                      | идентификатор изменения           |
| parent_id               | идентификатор измения родителя    |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
