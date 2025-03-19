## crm email addresses
#crm #crm_email_addresses

Содержит информацию о email.


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Структура PII.](#структура_PII)
4. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                                                                                                                                                                                      | Источники                                                                                                                                       |
|---------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [crm_email_addresses](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_email_addresses), [PII-crm_email_addresses](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/PII/crm_email_addresses)       | [raw-crm_email_addresses](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_email_addresses) |
| PREPROD | [crm_email_addresses](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/crm/crm_email_addresses), [PII-crm_email_addresses](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/crm/PII/crm_email_addresses) | [raw-crm_email_addresses](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_email_addresses) |


### Структура

| Поле                     | Описание                          |
|--------------------------|-----------------------------------|
| date_created_ts          | дата создания, utc                |
| date_created_dttm_local  | дата создания, local tz           |
| date_modified_ts         | дата время модификации, utc       |
| date_modified_dttm_local | дата время модификацииа, local tz |
| deleted                  | была ли удалена                   |
| email_address_hash       | email                             |
| email_address_caps_hash  | email в верхнем регистре          |
| crm_email_id             | id email , `PK`                   |
| invalid_email            | валиден ли email?                 |
| opt_out                  | сделан ли OPT-OUT?                |


### Структура - PII

| Поле               | Описание                 |
|--------------------|--------------------------|
| email_address      | email                    |
| email_address_caps | email в верхнем регистре |
| crm_email_id       | id email , `PK`          |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
