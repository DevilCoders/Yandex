## crm leads audit
#crm #crm_leads_audit

Содержит аудит по изменению данных [лидов](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_leads).


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Структура PII.](#структура_PII)
4. [Загрузка.](#загрузка)


### Расположение данных

| Контур    | Расположение данных   | Источники |
| --------- | -------------------   | --------- |
| PROD      | [crm_leads](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_leads_audit), [PII-crm_leads](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/PII/crm_leads_audit) | [raw-crm_leads](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_leads_audit) |
| PREPROD   | [crm_leads](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/crm/crm_leads_audit), [PII-crm_leads](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/crm/PII/crm_leads_audit)| [raw-crm_leads](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_leads_audit) |


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


### Структура - PII

Содержит изменения стобцов:
- crm_account_name - название аккаунта
- alt_address_country - дополнительный адрес, страна
- alt_address_postalcode - дополнительный адрес, индекс
- alt_address_state - дополнительный адрес, государство
- alt_address_street - дополнительный адрес, улица
- assistant - ассистент
- assistant_phone - телефон ассистента
- billing_address_country - адрес для выставления счетов, страна
- billing_address_postalcode - адрес для выставления счетов, индекс
- billing_address_state - адрес для выставления счетов, государство
- billing_address_street - адрес для выставления счетов, улица
- birthdate - дата рождения
- facebook - id facebook
- first_name - Имя
- googleplus - googleplus
- last_name - Фамилия
- phone_fax - факс
- phone_home - домашний телефон
- phone_mobile - мобильный телефон
- phone_other - другой телефон
- phone_work - телефон рабочий
- primary_address_country - основной адрес,  страна
- primary_address_postalcode - основной адрес, индекс
- primary_address_state - основной адрес, государство
- primary_address_street - основной адрес, улица
- salutation - приветствие
- shipping_address_country - адрес доставки, страна
- shipping_address_postalcode - адрес доставки, индекс
- shipping_address_state - адрес доставки, государство
- shipping_address_street - адрес доставки, улица
- title - заголовок
- twitter - аккаунт в twitter
- yandex_login - логин яндекса

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
